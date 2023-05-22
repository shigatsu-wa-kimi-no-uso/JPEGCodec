/*
* Encoder.cpp
* Written by kiminouso, 2023/05/01
*/
#include <iomanip>
#include <iostream>
#include "Encoder.h"
#include "Quantizer.h"
#include "DCT.h"
#include "IntHuffman.h"
#include "JPEGFileBuilder.h"
#include "UtilFunc.h"

Encoder::SymbolTable::SymbolTable(int entryCount) :
	_freqMap(new size_t[entryCount]{ 0 }),
	_codeMap(new BitString[entryCount]),
	_count(entryCount){}

const int Encoder::SymbolTable::count() const{
	return _count;
}

size_t& Encoder::SymbolTable::frequency(const int symbol) const{
	return _freqMap[symbol];
}

BitString& Encoder::SymbolTable::code(const int symbol){
	return _codeMap[symbol];
}

const BitString& Encoder::SymbolTable::code(const int symbol) const{
	return _codeMap[symbol];
}

size_t* Encoder::SymbolTable::frequencyMapPtr() const{
	return _freqMap;
}

Encoder::SymbolTable::~SymbolTable(){
	delete[] _freqMap;
	delete[] _codeMap;
}

void Encoder::_makeBoundaryMCU(const Matrix<YCbCr>& ycbcrData, MCU& mcu, int pos_x, int pos_y){
	int stride_r = _subsampFact.factor_v;
	int stride_c = _subsampFact.factor_h;
	int row_cnt = BLOCK_ROWCNT * stride_r;
	int col_cnt = BLOCK_COLCNT * stride_c;
	int imgbound_col = ycbcrData.column_cnt;
	int imgbound_row = ycbcrData.row_cnt;

	//写入Y
	for (int r = 0; r < row_cnt; ++r) {
		int imgpos_y = pos_y + r >= imgbound_row ? imgbound_row - 1 : pos_y + r;
		for (int c = 0; c < col_cnt; ++c) {
			int imgpos_x = pos_x + c >= imgbound_col ? imgbound_col - 1 : pos_x + c;
			int y_blockSel = stride_c * (r / BLOCK_ROWCNT) + c / BLOCK_ROWCNT;
			int y_unitSel_r = r % BLOCK_ROWCNT;
			int y_unitSel_c = c % BLOCK_COLCNT;
			mcu.y[y_blockSel][0][y_unitSel_r][y_unitSel_c] = ycbcrData[imgpos_y][imgpos_x].Y;
		}
	}
	//写入CbCr
	for (int r = 0; r < row_cnt; r += stride_r) {
		int imgpos_y = pos_y + r >= imgbound_row ? imgbound_row - 1 : pos_y + r;
		for (int c = 0; c < col_cnt; c += stride_c) {
			int imgpos_x = pos_x + c >= imgbound_col ? imgbound_col - 1 : pos_x + c;
			mcu.cb[0][r / stride_r][c / stride_c] = ycbcrData[imgpos_y][imgpos_x].Cb;
			mcu.cr[0][r / stride_r][c / stride_c] = ycbcrData[imgpos_y][imgpos_x].Cr;
		}
	}
}

void Encoder::_allocMCU(MCU& mcu){
	int stride_r = _subsampFact.factor_v;
	int stride_c = _subsampFact.factor_h;
	mcu.y = new Block * [stride_r * stride_c];
	for (int i = 0; i < stride_r * stride_c; ++i) {
		mcu.y[i] = new Block[1];
	}
	mcu.cb = new Block[1];
	mcu.cr = new Block[1];
}

void Encoder::_deallocMCU(MCU& mcu){
	int stride_r = _subsampFact.factor_v;
	int stride_c = _subsampFact.factor_h;
	for (int i = 0; i < stride_r * stride_c; ++i) {
		delete[] mcu.y[i];
	}
	delete[] mcu.y;
	delete[] mcu.cb;
	delete[] mcu.cr;
}

void Encoder::_makeOneMCU(const Matrix<YCbCr>& ycbcrData, MCU& mcu, int pos_x, int pos_y){
	int stride_r = _subsampFact.factor_v;
	int stride_c = _subsampFact.factor_h;
	int row_cnt = BLOCK_ROWCNT * stride_r;
	int col_cnt = BLOCK_COLCNT * stride_c;
	//写入Y
	for (int r = 0; r < row_cnt; ++r) {
		int imgpos_y = pos_y + r;
		int y_unitSel_r = r % BLOCK_ROWCNT;
		for (int c = 0; c < col_cnt; ++c) {
			int imgpos_x = pos_x + c;
			int y_blockSel = stride_c * (r / BLOCK_ROWCNT) + c / BLOCK_ROWCNT;
			int y_unitSel_c = c % BLOCK_COLCNT;
			mcu.y[y_blockSel][0][y_unitSel_r][y_unitSel_c] = ycbcrData[imgpos_y][imgpos_x].Y;
		}
	}
	//写入CbCr
	for (int r = 0; r < row_cnt; r += stride_r) {
		int imgpos_y = pos_y + r;
		for (int c = 0; c < col_cnt; c += stride_c) {
			int imgpos_x = pos_x + c;
			mcu.cb[0][r / stride_r][c / stride_c] = ycbcrData[imgpos_y][imgpos_x].Cb;
			mcu.cr[0][r / stride_r][c / stride_c] = ycbcrData[imgpos_y][imgpos_x].Cr;
		}
	}
}

void Encoder::_updateMCU(MCU& oldMCU, MCU& newMCU){
	MCU& mcu = oldMCU;
	_deallocMCU(oldMCU);
	mcu.y = newMCU.y;
	mcu.cb = newMCU.cb;
	mcu.cr = newMCU.cr;
}

//对每个值减去128以便进行FDCT操作

void Encoder::_lshift128(Block& block) {
	for (int r = 0; r < BLOCK_ROWCNT; ++r) {
		for (int c = 0; c < BLOCK_COLCNT; ++c) {
			block[r][c] -= 128;
		}
	}
}

void Encoder::_bitEncodeOneBlock(const Block& input, BitCodeArray& output, DPCM& dpcmEncoder){
	RLCode codeBuf[256];
	//问题: 跨越不同类型的Block(如Y到Cb)时,差分编码是否需要重置?
	int DCDiff = dpcmEncoder.nextDiff((int)input[0][0]);
	codeBuf[0].zeroCnt = 0;
	codeBuf[0].value = DCDiff;
	int count = 0;
	int zigzagged[BLOCK_COLCNT * BLOCK_ROWCNT];
	getZigzaggedSequence(input, zigzagged);
#ifdef TRACE
	printf("zigzagged:\n");
	for (int i = 0; i < 64; ++i) {
		printf("%d ", zigzagged[i]);
	}
	putchar('\n');
#endif
	RunLengthCodec::encode(&zigzagged[1], BLOCK_COLCNT * BLOCK_ROWCNT - 1, &codeBuf[1], &count);
	count++;
	for (int i = 0; i < count; ++i) {
		BitString bitString = BitCodec::getBitString(codeBuf[i].value);
		BitCode bitCode;
		bitCode.zeroCnt = codeBuf[i].zeroCnt;
		bitCode.bits = bitString.value();	//为EOB时,bits是空的(长度为0)
		bitCode.bitLength = bitString.length();
		output.push_back(bitCode);
	}
#ifdef TRACE
	printf("RunLengthCodec:\n");
	for (int i = 0; i < count; ++i) {
		printf("(%d, %d, ", output[i].zeroCnt, output[i].bitLength);
		std::cout << RLEcodeBuf[i].value << ", " << BitEncoder::getBitString(RLEcodeBuf[i].value) << ")";
	}
	putchar('\n');
#endif
}

void Encoder::_accumulateFrequency(const BitCodeArray& input, SymbolTable& tbl_DC, SymbolTable& tbl_AC)
{
	size_t len = input.size();
	tbl_DC.frequency(input[0].codedUnit)++;
	for (size_t i = 1; i < len; ++i) {
		tbl_AC.frequency(input[i].codedUnit)++;
	}
}

void Encoder::_countOneUnitFreq(const BitCodeUnit& bitCodeUnit, SymbolTable& tbl_Y_DC, SymbolTable& tbl_Y_AC, SymbolTable& tbl_CbCr_DC, SymbolTable& tbl_CbCr_AC)
{
	for (int i = 0; i < _subsampFact.factor_h * _subsampFact.factor_v; ++i) {
		_accumulateFrequency(bitCodeUnit.y[i], tbl_Y_DC, tbl_Y_AC);
	}
	_accumulateFrequency(bitCodeUnit.cb, tbl_CbCr_DC, tbl_CbCr_AC);
	_accumulateFrequency(bitCodeUnit.cr, tbl_CbCr_DC, tbl_CbCr_AC);
}

void Encoder::_countFrequency(const Matrix<BitCodeUnit>& bitCodeMatrix, SymbolTable& tbl_Y_DC, SymbolTable& tbl_Y_AC, SymbolTable& tbl_CbCr_DC, SymbolTable& tbl_CbCr_AC)
{
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			_countOneUnitFreq(
				bitCodeMatrix[r][c],
				tbl_Y_DC,
				tbl_Y_AC,
				tbl_CbCr_DC,
				tbl_CbCr_AC);
		}
	}
}

void Encoder::_bitEncodeOneMCU(const MCU& mcu, BitCodeUnit& codeUnit)
{
	int i, j;
	codeUnit.y.resize(_subsampFact.factor_v * _subsampFact.factor_h);
	for (i = 0; i < _subsampFact.factor_v; ++i) {
		for (j = 0; j < _subsampFact.factor_h; ++j) {
			_bitEncodeOneBlock(*mcu.y[i * _subsampFact.factor_h + j], codeUnit.y[i * _subsampFact.factor_h + j], _dpcmEncoders[0]);
		}
	}
	_bitEncodeOneBlock(*mcu.cb, codeUnit.cb, _dpcmEncoders[1]);
	_bitEncodeOneBlock(*mcu.cr, codeUnit.cr, _dpcmEncoders[2]);
}

void Encoder::_bitEncode(Matrix<BitCodeUnit>& bitCodeMatrix)
{
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			_bitEncodeOneMCU((*_MCUs)[r][c], bitCodeMatrix[r][c]);
		}
	}
}

void Encoder::_generateHuffmanTable(SymbolTable& symbolTable, HuffmanTable& huffTable)
{
	IntHuffman huffmanUtil;
	std::vector<int> codeLengthTable;
	std::vector<std::vector<BitString>> bitStringTable;
	huffmanUtil.setFrequencyMap(symbolTable.frequencyMapPtr(), symbolTable.count());
	huffmanUtil.buildTree();
	huffmanUtil.getLengthCountTable(codeLengthTable, HUFFMAN_CODE_LENGTH_LIMIT);
	huffmanUtil.getCanonicalTable(codeLengthTable, huffTable);
	huffmanUtil.getCanonicalCodes(huffTable, bitStringTable);
	size_t lengthCnt = bitStringTable.size();
	for (size_t i = 1; i < lengthCnt; ++i) {
		size_t sublistSize = bitStringTable[i].size();
		for (size_t j = 0; j < sublistSize; ++j) {
			symbolTable.code(huffTable[i][j]) = bitStringTable[i][j];
		}
	}
}

void Encoder::_huffmanEncodeOneBlock(const BitCodeArray& bitCodes, const SymbolTable& tbl_DC, const SymbolTable& tbl_AC, BitWriter& writer)
{
	size_t len = bitCodes.size();
	writer.write(tbl_DC.code(bitCodes[0].codedUnit));
	writer.write(BitString(bitCodes[0].bits, bitCodes[0].bitLength));
	for (size_t i = 1; i < len; ++i) {
		writer.write(tbl_AC.code(bitCodes[i].codedUnit));
		writer.write(BitString(bitCodes[i].bits, bitCodes[i].bitLength));
	}
#ifdef TRACE
	printf("Huffman:\n");
	std::cout << "(" << (int)bitCodes[0].codedUnit << "--" << tbl_DC.code(bitCodes[0].codedUnit) << ")";
	for (int i = 1; i < len; ++i) {
		std::cout << "(" << (int)bitCodes[i].codedUnit << "--" << tbl_AC.code(bitCodes[i].codedUnit) << ")";
	}
	putchar('\n');
#endif
}

void Encoder::_huffmanEncodeOneUnit(const BitCodeUnit& bitCodeUnit, const SymbolTable& tbl_Y_DC, const SymbolTable& tbl_Y_AC, const SymbolTable& tbl_CbCr_DC, const SymbolTable& tbl_CbCr_AC, BitWriter& writer)
{
	for (int i = 0; i < _subsampFact.factor_h * _subsampFact.factor_v; ++i) {
		_huffmanEncodeOneBlock(bitCodeUnit.y[i], tbl_Y_DC, tbl_Y_AC, writer);
	}
	_huffmanEncodeOneBlock(bitCodeUnit.cb, tbl_CbCr_DC, tbl_CbCr_AC, writer);
	_huffmanEncodeOneBlock(bitCodeUnit.cr, tbl_CbCr_DC, tbl_CbCr_AC, writer);
}

void Encoder::_reserveLastSymbol(SymbolTable& table, const DWORD lastSymbol)
{
	table.frequency(lastSymbol) = 1;
}

void Encoder::_removeReservedSymbol(HuffmanTable& table)
{
	size_t len = table.size();
	size_t i = len - 1;
	//不考虑i越界的情况,正常情况下不可能越界,否则说明HuffmanTable本身有问题
	while (table[i].empty()) {
		--i;
	}
	table[i].pop_back();
}

void Encoder::_printHuffmanTable(const SymbolTable& symbolTable,const HuffmanTable& table, const char* description)
{
	printf("%s\n", description);
	printf("%8s%5s%18s\n", "symbol", "freq", "code");
	for (int i = 0; i < 257; ++i) {
		if (symbolTable.frequency(i) != 0) {
			printf("%8d%5d", i, symbolTable.frequency(i));
			std::cout << std::setw(18) << symbolTable.code(i) << "\n";
		}
	}
	printf("%8s%18s%5s\n", "symbol", "code", "len");
	for (int i = 1; i < table.size(); ++i) {
		for (int j = 0; j < table[i].size(); ++j) {
			std::cout << std::setw(8) << table[i][j] << std::setw(18) << symbolTable.code(table[i][j]) << std::setw(5) << i << "\n";
		}
	}
	printf("%8s%8s\n", "len", "freq");
	for (int i = 1; i < table.size(); ++i) {
		int freq = 0;
		for (int j = 0; j < table[i].size(); ++j) {
			freq += symbolTable.frequency(table[i][j]);
		}
		std::cout << std::setw(8) << i << std::setw(8) << freq << "\n";
	}
	putchar('\n');
}

void Encoder::_huffmanEncode(const Matrix<BitCodeUnit>& bitCodeMatrix, std::vector<BYTE>& output)
{
	SymbolTable table_Y_DC(257);
	SymbolTable table_Y_AC(257);
	SymbolTable table_CbCr_DC(257);
	SymbolTable table_CbCr_AC(257);
	BitWriter writer(output);
	_countFrequency(bitCodeMatrix, table_Y_DC, table_Y_AC, table_CbCr_DC, table_CbCr_AC);
	//补点,补充值为256的符号(BYTE类型的符号不可能取到该值),频数设为1, 在使用IntHuffman生成哈夫曼编码时,内部的排序策略会使得符号'256'被放在树的最后一个节点上,生成的编码为全1
	//补充这个点以保证任何值为[0,255]的符号对应的编码不为全1
	_reserveLastSymbol(table_Y_DC, 256);
	_reserveLastSymbol(table_Y_AC, 256);
	_reserveLastSymbol(table_CbCr_DC, 256);
	_reserveLastSymbol(table_CbCr_AC, 256);
	_generateHuffmanTable(table_Y_DC, _huffTable_Y_DC);
	_generateHuffmanTable(table_Y_AC, _huffTable_Y_AC);
	_generateHuffmanTable(table_CbCr_DC, _huffTable_CbCr_DC);
	_generateHuffmanTable(table_CbCr_AC, _huffTable_CbCr_AC);
#ifdef DEBUG
	_printHuffmanTable(table_Y_DC, _huffTable_Y_DC, "Y_DC");
	_printHuffmanTable(table_Y_AC, _huffTable_Y_AC, "Y_AC");
	_printHuffmanTable(table_CbCr_DC, _huffTable_CbCr_DC, "CbCr_DC");
	_printHuffmanTable(table_CbCr_AC, _huffTable_CbCr_AC, "CbCr_AC");
#endif
	//删除前面补充的值为'256'的符号,后续向jpeg文件中写入的哈夫曼表应当是没有补点的
	_removeReservedSymbol(_huffTable_Y_DC);
	_removeReservedSymbol(_huffTable_Y_AC);
	_removeReservedSymbol(_huffTable_CbCr_DC);
	_removeReservedSymbol(_huffTable_CbCr_AC);
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			_huffmanEncodeOneUnit(
				bitCodeMatrix[r][c],
				table_Y_DC,
				table_Y_AC,
				table_CbCr_DC,
				table_CbCr_AC,
				writer);
		}
	}
	writer.fillIncompleteByteWithOne();
}

void Encoder::makeMCUs(const Matrix<YCbCr>& ycbcrData,const SubsampFact subsampFact)
{
	_subsampFact = subsampFact;
	_width = (WORD)ycbcrData.column_cnt;
	_height = (WORD)ycbcrData.row_cnt;
	int mcus_col = (ALIGN(ycbcrData.column_cnt, _subsampFact.factor_h + 2)) / (_subsampFact.factor_h * BLOCK_COLCNT);
	int mcus_row = (ALIGN(ycbcrData.row_cnt, _subsampFact.factor_v + 2)) / (_subsampFact.factor_v * BLOCK_ROWCNT);
	int mcu_colUnitCnt = BLOCK_COLCNT * subsampFact.factor_h;
	int mcu_rowUnitCnt = BLOCK_ROWCNT * subsampFact.factor_v;
	_MCUs = new Matrix<MCU>(mcus_row, mcus_col);
	int r, c;
	for (r = 0; r < mcus_row - 1; ++r) {
		for (c = 0; c < mcus_col - 1; ++c) {
			_allocMCU((*_MCUs)[r][c]);
			_makeOneMCU(ycbcrData, (*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
		}
		_allocMCU((*_MCUs)[r][c]);
		_makeBoundaryMCU(ycbcrData, (*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
	}
	for (c = 0; c < mcus_col; ++c) {
		_allocMCU((*_MCUs)[r][c]);
		_makeBoundaryMCU(ycbcrData, (*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
	}
}

void Encoder::doFDCT()
{
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			//在MCU矩阵中选择一个MCU, 并分配新的MCU
			MCU& mcu = (*_MCUs)[r][c];
			MCU newMCU;
			_allocMCU(newMCU);
			//对Y块进行FDCT
			for (DWORD i = 0; i < _subsampFact.factor_v; ++i) {
				for (DWORD j = 0; j < _subsampFact.factor_h; ++j) {
					Block& input = *mcu.y[i * _subsampFact.factor_h + j];
					Block& output = *newMCU.y[i * _subsampFact.factor_h + j];
					_lshift128(input);
					DCT::forwardDCT(input, output);
				}
			}
			//对Cb,Cr块进行FDCT
			_lshift128(*mcu.cb);
			DCT::forwardDCT(*mcu.cb, *newMCU.cb);

			_lshift128(*mcu.cr);
			DCT::forwardDCT(*mcu.cr, *newMCU.cr);

			//更新到MCU矩阵
			_updateMCU(mcu, newMCU);
		}
	}
}

void Encoder::quantize(float qualityFactor)
{
	Quantizer::generateUserTable(qualityFactor);
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			MCU& mcu = (*_MCUs)[r][c];
			for (DWORD i = 0; i < _subsampFact.factor_v; ++i) {
				for (DWORD j = 0; j < _subsampFact.factor_h; ++j) {
					Block& input = *mcu.y[i * _subsampFact.factor_h + j];
					Block& output = input;
					Quantizer::quantize(input, Quantizer::STD_QTABLE_LUMA, output);
				}
			}
			Quantizer::quantize(*mcu.cb, Quantizer::STD_QTABLE_CHROMA, *mcu.cb);
			Quantizer::quantize(*mcu.cr, Quantizer::STD_QTABLE_CHROMA, *mcu.cr);
		}
	}
}

void Encoder::encode()
{
	Matrix<BitCodeUnit> bitCodeMatrix(_MCUs->row_cnt, _MCUs->column_cnt);
	_bitEncode(bitCodeMatrix);
	_huffmanEncode(bitCodeMatrix, _codedBytes);
}

void Encoder::makeJPGFile(const char* fileName)
{
	JPEGFileBuilder file;
	file.setHeight(_height)
		.setWidth(_width)
		.setQuantTable(0, Quantizer::getQuantTable(Quantizer::STD_QTABLE_LUMA))
		.setQuantTable(1, Quantizer::getQuantTable(Quantizer::STD_QTABLE_CHROMA))
		.setSubsampFact(_subsampFact)
		.setHuffmanTable(0, _huffTable_Y_DC, HTableType::DC)
		.setHuffmanTable(0, _huffTable_Y_AC, HTableType::AC)
		.setHuffmanTable(1, _huffTable_CbCr_DC, HTableType::DC)
		.setHuffmanTable(1, _huffTable_CbCr_AC, HTableType::AC)
		.setCmptAdoptedHTable(Component::LUMA, HTableType::DC, 0)
		.setCmptAdoptedHTable(Component::LUMA, HTableType::AC, 0)
		.setCmptAdoptedHTable(Component::CHROMA_B, HTableType::DC, 1)
		.setCmptAdoptedHTable(Component::CHROMA_B, HTableType::AC, 1)
		.setCmptAdoptedHTable(Component::CHROMA_R, HTableType::DC, 1)
		.setCmptAdoptedHTable(Component::CHROMA_R, HTableType::AC, 1)
		.setCodedBytes(_codedBytes)
		.makeJPGFile(fileName);
}
