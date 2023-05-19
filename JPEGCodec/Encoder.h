#pragma once
/*
* CodingUnits.h
* Written by kiminouso, 2023/05/01
*/
#ifndef CodingUnits_h__
#define CodingUnits_h__
#include <vector>
#include "typedef.h"
#include "Quantizer.h"
#include "CodingUtil.h"
#include "DCT.h"
#include "IntHuffman.h"
#include "BitWriter.h"

class Encoder {
private:
	class SymbolTable {
	private:
		size_t* _freqMap;
		BitString* _codeMap;
		int _count;
	public:
		SymbolTable(int entryCount):
			_freqMap(new size_t[entryCount]{0}),
			_codeMap(new BitString[entryCount]),
			_count(entryCount){}
		const int count() const {
			return _count;
		}
		size_t& frequency(const int symbol) const {
			return _freqMap[symbol];
		}
		BitString& code(const int symbol) const {
			return _codeMap[symbol];
		}
		size_t* frequencyMapPtr() const {
			return _freqMap;
		}
		~SymbolTable() {
			delete[] _freqMap;
			delete[] _codeMap;
		}
	};

	struct BitCode {
		union {
			BYTE bitLength : 4;
			BYTE zeroCnt : 4;
			BYTE codedUnit;
		};
		DWORD bits;
	};
	using BitCodeArray = std::vector<BitCode>;
	using HuffmanTable = std::vector<std::vector<DWORD>>;

	struct BitCodeUnit {
		std::vector<BitCodeArray> y;
		BitCodeArray cb;
		BitCodeArray cr;
	};

	SubsampFact _subsampFact;

	/*MCU的结构:
	* 在图像中的结构:
	* 由水平采样因子h 和 垂直采样因子v决定
	* h/v=1/1
	* +-------+-------+-------+
	* |  MCU1 |  MCU2 |  ...  |
	* |  8x8  |  8x8  |       |
	* +-------+-------+-------+
	* Single MCU:
	* +-------+ +-------+ +-------+
	* | Block | | Block | | Block |
	* |  Y1   | |  Cb   | |  Cr   |
	* +-------+ +-------+ +-------+
	* h/v=1/2
	* +-------+-------+-------+
	* |  MCU1 |  MCU2 |  ...  |
	* |  8x16 |  8x16 |       |
	* +-------+-------+-------+
	* Single MCU:
	* +-------+   +-------+ +-------+
	* | Block |   | Block | | Block |
	* |  Y1   |   |  Cb   | |  Cr   |
	* +-------+   +-------+ +-------+
	* | Block |
	* |  Y2   |
	* +-------+
	* h/v=2/2
	* +-------+-------+-------+
	* |  MCU1 |  MCU2 |  ...  |
	* | 16x16 | 16x16 |       |
	* +-------+-------+-------+
	* Single MCU:
	* +-------+-------+   +-------+ +-------+
	* | Block | Block |   | Block | | Block |
	* |  Y1   |  Y2   |   |  Cb   | |  Cr   |
	* +-------+-------+   +-------+ +-------+
	* | Block | Block |
	* |  Y3   |  Y4   |
	* +-------+-------+
	* 一维存储结构:
	* +-------+-------+-------+-------+-------+-------+
	* | Block | Block |  ...  | Block | Block | Block |
	* |  Y1   |   Y2  |       |   Yn  |  Cb   |   Cr  |
	* +-------+-------+-------+-------+-------+-------+
	*/
	Matrix<MCU>* _MCUs;

	HuffmanTable  _huffTable_Y_DC;
	HuffmanTable  _huffTable_Y_AC;
	HuffmanTable  _huffTable_CbCr_DC;
	HuffmanTable  _huffTable_CbCr_AC;
	WORD _width;
	WORD _height;
	std::vector<BYTE> _codedBytes;

	void _makeBoundaryMCU(Matrix<YCbCr>* ycbcrData, MCU* mcu, int pos_x, int pos_y) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		int row_cnt = BLOCK_ROWCNT * stride_r;
		int col_cnt = BLOCK_COLCNT * stride_c;
		int imgbound_col = ycbcrData->column_cnt;
		int imgbound_row = ycbcrData->row_cnt;

		//写入Y
		for (int r = 0; r < row_cnt; ++r) {
			for (int c = 0; c < col_cnt; ++c) {
				int imgpos_y = pos_y + r >= imgbound_col ? imgbound_col - 1 : pos_y + r;
				int imgpos_x = pos_x + c >= imgbound_row ? imgbound_row - 1 : pos_x + c;
				int y_blockSel = stride_c * (r / BLOCK_ROWCNT) + c / BLOCK_ROWCNT;
				int y_unitSel_r = r % BLOCK_ROWCNT;
				int y_unitSel_c = c % BLOCK_COLCNT;
				(*mcu->y[y_blockSel])[y_unitSel_c][y_unitSel_c] = (*ycbcrData)[imgpos_y][imgpos_x].Y;
			}
		}
		//写入CbCr
		for (int r = 0; r < row_cnt; r += stride_r) {
			for (int c = 0; c < col_cnt; c += stride_c) {
				int imgpos_y = pos_y + r >= imgbound_col ? imgbound_col - 1 : pos_y + r;
				int imgpos_x = pos_x + c >= imgbound_row ? imgbound_row - 1 : pos_x + c;
				(*mcu->cb)[r / stride_r][c / stride_c] = (*ycbcrData)[imgpos_y][imgpos_x].Cb;
				(*mcu->cr)[r / stride_r][c / stride_c] = (*ycbcrData)[imgpos_y][imgpos_x].Cr;
			}
		}
	}

	//分配一块MCU
	void _allocMCU(MCU* pMCU) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		pMCU->y = new Block * [stride_r * stride_c];
		for (int i = 0; i < stride_r * stride_c; ++i) {
			pMCU->y[i] = new Block[1];
		}
		pMCU->cb = new Block[1];
		pMCU->cr = new Block[1];
	}

	void _deallocMCU(MCU* pMCU) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		pMCU->y = new Block * [stride_r * stride_c];
		for (int i = 0; i < stride_r * stride_c; ++i) {
			delete[] pMCU->y[i];
		}
		delete[] pMCU->cb;
		delete[] pMCU->cr;
	}

	void _makeOneMCU(Matrix<YCbCr>* ycbcrData, MCU* mcu, int pos_x, int pos_y) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		int row_cnt = BLOCK_ROWCNT * stride_r;
		int col_cnt = BLOCK_COLCNT * stride_c;
		//写入Y
		for (int r = 0; r < row_cnt; ++r) {
			for (int c = 0; c < col_cnt; ++c) {
				int imgpos_y = pos_y + r;
				int imgpos_x = pos_x + c;
				int y_blockSel = stride_c * (r / BLOCK_ROWCNT) + c / BLOCK_ROWCNT;
				int y_unitSel_r = r % BLOCK_ROWCNT;
				int y_unitSel_c = c % BLOCK_COLCNT;
				(*mcu->y[y_blockSel])[y_unitSel_c][y_unitSel_c] = (*ycbcrData)[imgpos_y][imgpos_x].Y;
			}
		}
		//写入CbCr
		for (int r = 0; r < row_cnt; r+=stride_r) {
			for (int c = 0; c < col_cnt; c+=stride_c) {
				int imgpos_y = pos_y + r;
				int imgpos_x = pos_x + c;
				(*mcu->cb)[r / stride_r][c / stride_c] = (*ycbcrData)[imgpos_y][imgpos_x].Cb;
				(*mcu->cr)[r / stride_r][c / stride_c] = (*ycbcrData)[imgpos_y][imgpos_x].Cr;
			}
		}
	}

	//注意:更新MCU矩阵时采用浅拷贝
	void _updateMCU(MCU* pNewMCU, int r, int c) {
		MCU* mcu = &(*_MCUs)[r][c];
		_deallocMCU(mcu);
		for (int i = 0; i < _subsampFact.factor_v; ++i) {
			for (int j = 0; j < _subsampFact.factor_h; ++j) {
				mcu->y[i * _subsampFact.factor_h + j] = pNewMCU->y[i * _subsampFact.factor_h + j];
			}
		}
		mcu->cb = pNewMCU->cb;
		mcu->cr = pNewMCU->cr;
	}

	//对每个值减去128以便进行FDCT操作
	void _lshift128(Block* block) {
		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				(*block)[r][c] -= 128;
			}
		}
	}

	//差分编码&游程编码
	//返回一个包装好的BitEncode结果数组BitCodeArray
	void _bitEncodeOneBlock(Block* input, BitCodeArray& output) {
		RLECode RLEcodeBuf[256];
		//问题: 跨越不同类型的Block(如Y到Cb)时,差分编码是否需要重置?
		int DCDiff = DPCM::nextDiff((*input)[0][0]);
		RLEcodeBuf[0].zeroCnt = 0;
		RLEcodeBuf[0].value = DCDiff;
		int count = 0;
		RLE::getRLECodes(input, &RLEcodeBuf[1], &count);
		count++;
		for (int i = 0; i < count; ++i) {
			BitString bitString = BitEncoder::getBitString(RLEcodeBuf[i].value);
			BitCode bitCode;
			bitCode.zeroCnt = RLEcodeBuf[i].zeroCnt;
			bitCode.bits = bitString.value();
			bitCode.bitLength = bitString.length();
			output.push_back(bitCode);
		}
	}

	//计算一个BitCodeArray所产生的CodedUnit符号的频数,一个BitCodeArray与一个block对应
	void _accumulateFrequency(const BitCodeArray& input, SymbolTable& tbl_DC,SymbolTable& tbl_AC) {
		int len = input.size();
		tbl_DC.frequency(input[0].codedUnit)++;
		for (int i = 1; i < len;++i) {
			tbl_AC.frequency(input[i].codedUnit)++;
		}
	}

	//计算一个MCU所产生的CodedUnit符号的频数,MCU所产生的BitCodeArray被封装在了BitCodeUnit中
	void _countOneUnitFreq(const BitCodeUnit& bitCodeUnit, SymbolTable& tbl_Y_DC, SymbolTable& tbl_Y_AC, SymbolTable& tbl_CbCr_DC, SymbolTable& tbl_CbCr_AC) {
		for (int i = 0; i < _subsampFact.factor_h * _subsampFact.factor_v; ++i) {
			_accumulateFrequency(bitCodeUnit.y[i], tbl_Y_DC,tbl_Y_AC);
		}
		_accumulateFrequency(bitCodeUnit.cb, tbl_CbCr_DC, tbl_CbCr_AC);
		_accumulateFrequency(bitCodeUnit.cr, tbl_CbCr_DC, tbl_CbCr_AC);
	}

	//计算所有MCU产生的CodedUnit符号的频数,以供生成huffman树
	void _countFrequency(
		const Matrix<BitCodeUnit>& bitCodeMatrix,
		SymbolTable& tbl_Y_DC,
		SymbolTable& tbl_Y_AC, 
		SymbolTable& tbl_CbCr_DC, 
		SymbolTable& tbl_CbCr_AC) {
		for (int r = 0; _MCUs->row_cnt; ++r) {
			for (int c = 0; _MCUs->column_cnt; ++c) {
				_countOneUnitFreq(
					bitCodeMatrix[r][c], 
					tbl_Y_DC,
					tbl_Y_AC,
					tbl_CbCr_DC,
					tbl_CbCr_AC);
			}
		}
	}

	//先Y,再Cb,再Cr
	//如果采样因子非1:1,则Y的块有多个,在一个MCU中从左往右从上到下遍历处理
	void _bitEncodeOneMCU(MCU* mcu, BitCodeUnit& codeUnit) {
		Block* input;
		int i, j;
		codeUnit.y.resize(_subsampFact.factor_v * _subsampFact.factor_h);
		for (i = 0; i < _subsampFact.factor_v; ++i) {
			for (j = 0; j < _subsampFact.factor_h; ++j) {
				input = mcu->y[i * _subsampFact.factor_h + j];
				_bitEncodeOneBlock(input, codeUnit.y[i * _subsampFact.factor_h + j]);
			}
		}
		input = mcu->cb;
		_bitEncodeOneBlock(input, codeUnit.cb);
		input = mcu->cr;
		_bitEncodeOneBlock(input, codeUnit.cr);
	}

	//对每个MCU进行差分编码及游程编码,并得到合并字节后的结果
	void _bitEncode(Matrix<BitCodeUnit>& bitCodeMatrix) {
		for (int r = 0; _MCUs->row_cnt; ++r) {
			for (int c = 0; _MCUs->column_cnt; ++c) {
				MCU* mcu = _MCUs[r][c];
				_bitEncodeOneMCU(mcu, bitCodeMatrix[r][c]);
			}
		}
	}

	void _getHuffmanTable(SymbolTable& symbolTable,HuffmanTable& huffTable) {
		IntHuffman huffmanUtil;
		std::vector<int> codeLengthTable;
		std::vector<std::vector<BitString>> bitStringTable;
		huffmanUtil.setFrequencyMap(symbolTable.frequencyMapPtr(), 257);
		huffmanUtil.buildTree();
		huffmanUtil.getLengthCountTable(codeLengthTable, HUFFMAN_CODE_LENGTH_LIMIT);
		huffmanUtil.getCanonicalTable(codeLengthTable, huffTable);
		huffmanUtil.getCanonicalCodes(huffTable, bitStringTable);
		int lengthCnt = bitStringTable.size();
		for (int i = 1; i < lengthCnt; ++i) {
			int sublistSize = bitStringTable[i].size();
			for (int j = 0; j < sublistSize; ++j) {
				symbolTable.code(huffTable[i][j]) = bitStringTable[i][j];
			}
		}
	}

	void _huffmanEncodeOneBlock(const BitCodeArray& bitCodes, const SymbolTable& tbl_DC, const SymbolTable& tbl_AC,BitWriter& writer) {
		int len = bitCodes.size();
		writer.write(tbl_DC.code(bitCodes[0].codedUnit));
		writer.write(bitCodes[0].bits);
		for (int i = 1; i < len;++i) {
			writer.write(tbl_AC.code(bitCodes[i].codedUnit));
			writer.write(bitCodes[i].bits);
		}
	}

	void _huffmanEncodeOneUnit(
		const BitCodeUnit& bitCodeUnit,
		const SymbolTable& tbl_Y_DC,
		const SymbolTable& tbl_Y_AC,
		const SymbolTable& tbl_CbCr_DC,
		const SymbolTable& tbl_CbCr_AC,
		BitWriter& writer) {
		for (int i = 0; i < _subsampFact.factor_h * _subsampFact.factor_v; ++i) {
			_huffmanEncodeOneBlock(bitCodeUnit.y[i], tbl_Y_DC, tbl_Y_AC,writer);
		}
		_huffmanEncodeOneBlock(bitCodeUnit.cb, tbl_CbCr_DC, tbl_CbCr_AC, writer);
		_huffmanEncodeOneBlock(bitCodeUnit.cr, tbl_CbCr_DC, tbl_CbCr_AC, writer);
	}

	void _huffmanEncode(Matrix<BitCodeUnit>& bitCodeMatrix, std::vector<BYTE>& output) {
		SymbolTable table_Y_DC(257);
		SymbolTable table_Y_AC(257);
		SymbolTable table_CbCr_DC(257);
		SymbolTable table_CbCr_AC(257);
		BitWriter writer(output);
		_countFrequency(bitCodeMatrix, table_Y_DC, table_Y_AC, table_CbCr_DC, table_CbCr_AC);
		_getHuffmanTable(table_Y_DC,_huffTable_Y_DC);
		_getHuffmanTable(table_Y_AC, _huffTable_Y_AC);
		_getHuffmanTable(table_CbCr_DC, _huffTable_CbCr_DC);
		_getHuffmanTable(table_CbCr_AC, _huffTable_CbCr_AC);
		for (int r = 0; _MCUs->row_cnt; ++r) {
			for (int c = 0; _MCUs->column_cnt; ++c) {
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

public:
	Encoder() {}

	/*
	CodingUnits(int linecnt, int colcnt, SubsampFact _subsampFact)
		:_subsampFact(_subsampFact),
		_mcucnt_col(ALIGN(colcnt, _subsampFact.factor_h * BLOCK_COLCNT) / (_subsampFact.factor_h * BLOCK_COLCNT)),
		_mcucnt_row(ALIGN(linecnt, _subsampFact.factor_v * BLOCK_ROWCNT) / (_subsampFact.factor_v * BLOCK_ROWCNT)) {
		_mcus = new MCU * [_mcucnt_row];
		for (int i = 0; i < _mcucnt_row; ++i) {
			_mcus[i] = new MCU[_mcucnt_col];
			_mcus[i]->y = new Block * [_subsampFact.factor_v];
			for (int j = 0; j < _mcucnt_row; ++j) {
				_mcus[i]->y[j] = new Block[_subsampFact.factor_h];
			}
			_mcus[i]->cb = new Block[1];
			_mcus[i]->cb[0][2][1];
		}
	}*/


	void makeMCUs(Matrix<YCbCr>* ycbcrData, SubsampFact subsampFact) {
		_subsampFact = subsampFact;
		_width = ycbcrData->column_cnt;
		_height = ycbcrData->row_cnt;
		int mcus_col = ALIGN(ycbcrData->column_cnt, _subsampFact.factor_h + 2) / (_subsampFact.factor_h * BLOCK_COLCNT);
		int mcus_row = ALIGN(ycbcrData->row_cnt, _subsampFact.factor_v + 2) / (_subsampFact.factor_v * BLOCK_ROWCNT);
		int mcu_colUnitCnt = BLOCK_COLCNT * subsampFact.factor_h;
		int mcu_rowUnitCnt = BLOCK_ROWCNT * subsampFact.factor_v;
		_MCUs = new Matrix<MCU>(mcus_row, mcus_col);
		int r,c;
		for (r = 0; r < mcus_row - 1; ++r) {
			for (c = 0; c < mcus_col - 1; ++c) {
				_allocMCU(&(*_MCUs)[r][c]);
				_makeOneMCU(ycbcrData, &(*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
			}
			_allocMCU(&(*_MCUs)[r][c]);
			_makeBoundaryMCU(ycbcrData, &(*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
		}
		for (c = 0; c < mcus_col; ++c) {
			_allocMCU(&(*_MCUs)[r][c]);
			_makeBoundaryMCU(ycbcrData, &(*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
		}
	}

	void doFDCT() {
		MCU* mcu;
		MCU* newMCU;
		Block* input;
		Block* output;
		for (int r = 0; _MCUs->row_cnt; ++r) {
			for (int c = 0; _MCUs->column_cnt; ++c) {
				//在MCU矩阵中选择一个MCU, 并分配新的MCU
				mcu = _MCUs[r][c];
				_allocMCU(newMCU);
				//对Y块进行FDCT
				for (int i = 0; i < _subsampFact.factor_v; ++i) {
					for (int j = 0; j < _subsampFact.factor_h; ++j) {
						input = mcu->y[i * _subsampFact.factor_h + j];
						output = new Block[1];
						_lshift128(input);
						DCT::forwardDCT(input, output);
						newMCU->y[i * _subsampFact.factor_h + j] = output;
					}
				}
				//对Cb,Cr块进行FDCT
				input = mcu->cb;
				output = new Block[1];
				_lshift128(input);		
				DCT::forwardDCT(input, output);
				newMCU->cb = output;
				input = mcu->cr;
				output = new Block[1];
				_lshift128(input);
				DCT::forwardDCT(input, output);
				newMCU->cr = output;
				//更新到MCU矩阵
				_updateMCU(newMCU, r, c);
			}
		}
	}

	void doQuantize() {
		MCU* mcu;
		Block* input;
		Block* output;
		for (int r = 0; _MCUs->row_cnt; ++r) {
			for (int c = 0; _MCUs->column_cnt; ++c) {
				mcu = _MCUs[r][c];
				for (int i = 0; i < _subsampFact.factor_v; ++i) {
					for (int j = 0; j < _subsampFact.factor_h; ++j) {
						input = mcu->y[i * _subsampFact.factor_h + j];
						output = input;
						Quantizer::quantize(input, output, Quantizer::STD_QTABLE_LUMA);
					}
				}
				input = mcu->cb;
				output = input;
				Quantizer::quantize(input, output, Quantizer::STD_QTABLE_CHROMA);
				input = mcu->cr;
				output = input;
				Quantizer::quantize(input, output, Quantizer::STD_QTABLE_CHROMA);
			}
		}
	}

	//过程:
	//DC分量差分编码 -->-+
	//                 	+-->对编码结果合并字节操作-->Huffman编码及序列化
	//AC分量游程编码 -->-+
	//+---------------BitEncode-----------------++---HuffmanEncode---+
	void encode() {
		Matrix<BitCodeUnit> bitCodeMatrix(_MCUs->row_cnt, _MCUs->column_cnt);
		_bitEncode(bitCodeMatrix);
		_huffmanEncode(bitCodeMatrix, _codedBytes);
	}

	void _writeJFIFHeader(FILE* file) {
		//JFIF Header
		JPEG_JFIFHeader jfifHeader = {
			big_endian(Marker::APP0),
			{
				big_endian((WORD)sizeof(JPEG_JFIFHeader::Info)),
				"JFIF",
				{1,1},
				JPEG_JFIFHeader::Info::Units::NONE,
				big_endian((WORD)1),
				big_endian((WORD)1),
				0,
				0
			}
		};
		fwrite(&jfifHeader, sizeof(jfifHeader), 1, file);
	}

	void _writeQuantTable(FILE* file, const BYTE* quantTable,const BYTE id) {
		WORD length = sizeof(JPEG_QTableHeader::Info) + BLOCK_COLCNT * BLOCK_ROWCNT;
		JPEG_QTableHeader qtHeader1 = {
			big_endian(Marker::DQT),
			{
				big_endian(length),
				id,
				JPEG_QTableHeader::Info::PREC_8BIT
			}
		};
		fwrite(&qtHeader1, sizeof(qtHeader1), 1, file);
		fwrite(quantTable, sizeof(BYTE), BLOCK_COLCNT * BLOCK_ROWCNT, file);
	}

	void _writeFrameHeader(FILE* file) {
		JPEG_FrameHeader_YCbCr header = {
			big_endian(Marker::BASELINE_DCT),
			{
				big_endian((WORD)sizeof(JPEG_FrameHeader_YCbCr::Info)),
				JPEG_FrameHeader_YCbCr::Info::PREC_8BIT,
				big_endian(_height),
				big_endian(_width),
				JPEG_FrameHeader_YCbCr::Info::COMPCNT_YUV,
				{
					{
						JPEG_FrameHeader_YCbCr::Info::ImgComponent::LUMA,
						0x11,
						0
					},
					{
						JPEG_FrameHeader_YCbCr::Info::ImgComponent::CHROMA_B,
						_subsampFact.factor,
						1
					},
					{
						JPEG_FrameHeader_YCbCr::Info::ImgComponent::CHROMA_R,
						_subsampFact.factor,
						2
					}
				}
			}
		};
		fwrite(&header, sizeof(header), 1, file);
	}

	void _writeHuffmanTable(FILE* file, BYTE id, BYTE type, HuffmanTable& table) {
		std::vector<BYTE> tableData;
		BYTE tableEntryLen[16];
		for (int i = 1; i <= HUFFMAN_CODE_LENGTH_LIMIT; ++i) {
			tableEntryLen[i - 1] = table[i].size();
			for (BYTE symbol : table[i]) {
				tableData.push_back(symbol);
			}
		}
		JPEG_HTableHeader header = {
			big_endian(Marker::DHT),
			{
				big_endian((WORD)(sizeof(JPEG_HTableHeader::Info) + tableData.size())),
				id,
				type,
				{0}
			}
		};
		memcpy(header.info.tableEntryLen, tableEntryLen, sizeof(tableEntryLen));
		fwrite(&header, sizeof(header), 1, file);
		fwrite(tableData.data(), sizeof(BYTE), tableData.size(), file);
	}

	void _writeScanHeader(FILE* file) {
		JPEG_ScanHeader_BDCT_YCbCr header = {
			big_endian(Marker::SOS),
			{
				big_endian((WORD)sizeof(JPEG_ScanHeader_BDCT_YCbCr::Info)),
				JPEG_ScanHeader_BDCT_YCbCr::Info::COMPCNT_YUV,
				{
					{
						JPEG_ScanHeader_BDCT_YCbCr::Info::ImgComponent::LUMA,
						0,
						0
					},
					{
						JPEG_ScanHeader_BDCT_YCbCr::Info::ImgComponent::CHROMA_B,
						1,
						1
					},
					{
						JPEG_ScanHeader_BDCT_YCbCr::Info::ImgComponent::CHROMA_R,
						1,
						1
					},
				},
				0,
				63,
				0,
				0
			}
		};
		fwrite(&header, sizeof(header), 1, file);
	}

	void _writeCodedBytes(FILE* file,const std::vector<BYTE>& bytes) {
		fwrite(bytes.data(), sizeof(bytes), 1, file);
	}

	void makeJPGFile(const char* fileName) {
		FILE* file = fopen(fileName, "wb");
		std::vector<BYTE> codedBytes;
		//SOI
		WORD SOImarker = big_endian(Marker::SOI);
		fwrite(&SOImarker,sizeof(SOImarker),1, file);
		_writeJFIFHeader(file);
		const BYTE* qtLuma = Quantizer::getQTable(Quantizer::STD_QTABLE_LUMA);
		const BYTE* qtChroma = Quantizer::getQTable(Quantizer::STD_QTABLE_CHROMA);
		_writeQuantTable(file,qtLuma,0);
		_writeQuantTable(file, qtChroma, 0);
		_writeFrameHeader(file);
		_writeHuffmanTable(file, 0, JPEG_HTableHeader::Info::DC, _huffTable_Y_DC);
		_writeHuffmanTable(file, 0, JPEG_HTableHeader::Info::AC, _huffTable_Y_AC);
		_writeHuffmanTable(file, 1, JPEG_HTableHeader::Info::DC, _huffTable_CbCr_DC);
		_writeHuffmanTable(file, 1, JPEG_HTableHeader::Info::AC, _huffTable_CbCr_AC);
		_writeScanHeader(file);
		_writeCodedBytes(file, codedBytes);
		WORD EOFMarker = big_endian(Marker::EOI);
		fwrite(&EOFMarker, sizeof(EOFMarker), 1, file);
	}

	Matrix<MCU>* getMCUs() {
		return _MCUs;
	}
};
#endif // CodingUnits_h__