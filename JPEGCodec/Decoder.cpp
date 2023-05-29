/*
* Decoder.cpp
* Written by kiminouso, 2023/05/26
*/
#include "Decoder.h"
#include "UtilFunc.h"
#include "DCT.h"
#include "Quantizer.h"
#include "IntHuffman.h"

Decoder::SymbolTable::SymbolTable() :
	_count(0),
	_symbols(nullptr) {
}

Decoder::SymbolTable::SymbolTable(SymbolTable&& val) noexcept :
	_symbols(val._symbols),
	_count(val._count) {
	val._symbols = nullptr;
	_count = 0;
}

Decoder::SymbolTable::SymbolTable(const SymbolTable& val) noexcept :
	_symbols(new Symbol[val._count]),
	_count(val._count) {
	memcpy(_symbols, val._symbols, sizeof(Symbol) * _count);
}

Decoder::SymbolTable::SymbolTable(size_t symbolCnt) :
	_symbols(new Symbol[symbolCnt]),
	_count(symbolCnt) {
	memset(_symbols, 0, _count * sizeof(Symbol));
}

Decoder::SymbolTable::~SymbolTable() {
	delete[] _symbols;
}

void Decoder::SymbolTable::operator=(SymbolTable&& rgt) noexcept {
	_count = rgt._count;
	_symbols = rgt._symbols;
	rgt._count = 0;
	rgt._symbols = nullptr;
}

void Decoder::SymbolTable::operator=(const SymbolTable& rgt) noexcept {
	_count = rgt._count;
	_symbols = new Symbol[_count];
	memcpy(_symbols, rgt._symbols, sizeof(Symbol) * _count);
}

bool Decoder::SymbolTable::hasSymbol(const BitString& bitString) const {
	return _symbols[bitString.value()].length == bitString.length();
}

Decoder::Symbol& Decoder::SymbolTable::symbol(const BitString& bitString) {
	return _symbols[bitString.value()];
}

const Decoder::Symbol& Decoder::SymbolTable::symbol(const BitString& bitString) const {
	return _symbols[bitString.value()];
}

//分配一块MCU
void Decoder::_allocMCU(MCU& mcu) {
	int stride_r = _subsampFact.factor_v;
	int stride_c = _subsampFact.factor_h;
	mcu.y = new Block * [stride_r * stride_c];
	for (int i = 0; i < stride_r * stride_c; ++i) {
		mcu.y[i] = new Block[1];
	}
	mcu.cb = new Block[1];
	mcu.cr = new Block[1];
}

void Decoder::_deallocMCU(MCU& mcu) {
	int stride_r = _subsampFact.factor_v;
	int stride_c = _subsampFact.factor_h;
	for (int i = 0; i < stride_r * stride_c; ++i) {
		delete[] mcu.y[i];
	}
	delete[] mcu.y;
	delete[] mcu.cb;
	delete[] mcu.cr;
}

//注意:更新MCU矩阵时采用浅拷贝

void Decoder::_updateMCU(MCU& oldMCU, MCU& newMCU) {
	MCU& mcu = oldMCU;
	_deallocMCU(oldMCU);
	mcu.y = newMCU.y;
	mcu.cb = newMCU.cb;
	mcu.cr = newMCU.cr;
}

void Decoder::_initAllocMCUs() {
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			_allocMCU((*_MCUs)[r][c]);
		}
	}
}

void Decoder::_deallocMCUs() {
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			_deallocMCU((*_MCUs)[r][c]);
		}
	}
}

void Decoder::_matchSymbol(SymbolTable& table, BitReader& reader, Symbol& symbol) {
	BitString bs;
	do {
		bs.push_back(reader.readBit());
		if (bs.length() > HUFFMAN_CODE_LENGTH_LIMIT) {
			fprintf(stderr, "Cannot decode bitstring at scandata segment %#x.%d\n", reader.bytePosition(), reader.bitPosition());
			exit(1);
		}
	} while (!table.hasSymbol(bs));
	symbol = table.symbol(bs);
	table.symbol(bs).freq++;
}

void Decoder::_huffmanDecodeOneBlock(const int tableID_DC, const int tableID_AC, BitCodeArray& bitCodes, BitReader& reader) {
	int count = 0;
	bool readEOB = false;
	Symbol symbol;
	BitCode bitCode;
	_matchSymbol(_symbolTables[(int)HTableType::DC][tableID_DC], reader, symbol);
	bitCodes.reserve(24);
	bitCode.codedUnit = symbol.symbol;
	bitCode.bits = reader.readBits(bitCode.bitLength).value();
	bitCodes.push_back(bitCode);
	++count;
	while (count < 64 && !readEOB) {
		_matchSymbol(_symbolTables[(int)HTableType::AC][tableID_AC], reader, symbol);
		bitCode.codedUnit = symbol.symbol;
		bitCode.bits = reader.readBits(bitCode.bitLength).value();
		bitCodes.push_back(bitCode);
		count += bitCode.zeroCnt + 1;
		if (bitCode.codedUnit == BitCode::EOB) {
			readEOB = true;
		}
	}
	if (count > 64) {
		fprintf(stderr, "Corrupted block at %#x.%d (Block has more than 64 units)\n", reader.bytePosition(), reader.bitPosition());
	}
}

void Decoder::_huffmanDecodeOneMCU(BitCodeUnit& bitCodeUnit, BitReader& reader) {
	int i, j;
	bitCodeUnit.y.resize(_subsampFact.factor_v * _subsampFact.factor_h);
	for (i = 0; i < _subsampFact.factor_v; ++i) {
		for (j = 0; j < _subsampFact.factor_h; ++j) {
			_huffmanDecodeOneBlock(
				_cmptCfgs[(int)Component::LUMA].DC_HTableSel,
				_cmptCfgs[(int)Component::LUMA].AC_HTableSel,
				bitCodeUnit.y[i * _subsampFact.factor_h + j],
				reader);
		}
	}

	_huffmanDecodeOneBlock(
		_cmptCfgs[(int)Component::CHROMA_B].DC_HTableSel,
		_cmptCfgs[(int)Component::CHROMA_B].AC_HTableSel,
		bitCodeUnit.cb,
		reader);
	
	_huffmanDecodeOneBlock(
		_cmptCfgs[(int)Component::CHROMA_R].DC_HTableSel,
		_cmptCfgs[(int)Component::CHROMA_R].AC_HTableSel,
		bitCodeUnit.cr,
		reader);
}

void Decoder::_huffmanDecode(Matrix<BitCodeUnit>& bitCodeMatrix) {
	BitReader reader(_codedData);
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			_huffmanDecodeOneMCU(bitCodeMatrix[r][c], reader);
		}
	}
}

#ifdef STAT_SYMBOL_FREQ
void Decoder::_printSymbolTableStatistics() {
	for (int i = 0; i < (int)HTableType::MAXENUMVAL; ++i) {
		for (int j = 0; j < _symbolTables[i].size(); ++j) {
			printf("type:%d id:%d\n", i, j);
			printf("%6s%8s\n", "len", "freq");
			for (int l = 1; l < _bitStringTables[i][j].size(); ++l) {
				size_t freq = 0;
				for (int k = 0; k < _bitStringTables[i][j][l].size(); ++k) {
					assert(_symbolTables[i][j].hasSymbol(_bitStringTables[i][j][l][k]));
					freq += _symbolTables[i][j].symbol(_bitStringTables[i][j][l][k]).freq;
				}
				printf("%6d%8lld\n", l, freq);
			}
			putchar('\n');
		}
	}
}
#endif

void Decoder::_bitDecodeOneBlock(const BitCodeArray& bitCodes, Block& block, DPCM& dpcmDecoder, RLCode(&codes)[256]) {
	size_t count = bitCodes.size();
	//不可能超过256,否则jpg文件本身就有问题
	//RLCode codes[256];
	for (size_t i = 0; i < count; ++i) {
		codes[i].value = BitCodec::getValue(BitString(bitCodes[i].bits, bitCodes[i].bitLength));
		codes[i].zeroCnt = bitCodes[i].zeroCnt;
	}
	RunLengthCodec::decode(codes, count, 64, (int*)block);
	block[0][0] = dpcmDecoder.nextVal(block[0][0]);
}

void Decoder::_bitDecodeOneMCU(const BitCodeUnit& bitCodeUnit, MCU& mcu) {
	int i, j;
	BitCodeArray bitCodes;
	RLCode rlc[256];
	for (i = 0; i < _subsampFact.factor_v; ++i) {
		for (j = 0; j < _subsampFact.factor_h; ++j) {
			_bitDecodeOneBlock(bitCodeUnit.y[i * _subsampFact.factor_h + j], *mcu.y[i * _subsampFact.factor_h + j], _dpcmDecoders[0], rlc);
		#ifdef TRACE_RLC
			printf("Y[%d][%d]\n", i, j);
			for (int k = 0; k < bitCodeUnit.y[i * _subsampFact.factor_h + j].size(); ++k) {
				printf("(%d, %d)", rlc[k].zeroCnt, rlc[k].value);
			}
			putchar('\n');
		#endif
		}
	}
	_bitDecodeOneBlock(bitCodeUnit.cb, *mcu.cb, _dpcmDecoders[1], rlc);
#ifdef TRACE_RLC
	printf("Cb\n");
	for (int i = 0; i < bitCodeUnit.cb.size(); ++i) {
		printf("(%d, %d)", rlc[i].zeroCnt, rlc[i].value);
	}
	putchar('\n');
#endif
	_bitDecodeOneBlock(bitCodeUnit.cr, *mcu.cr, _dpcmDecoders[2], rlc);
#ifdef TRACE_RLC
	printf("Cr\n", i, j);
	for (int i = 0; i < bitCodeUnit.cr.size(); ++i) {
		printf("(%d, %d)", rlc[i].zeroCnt, rlc[i].value);
	}
	putchar('\n');
#endif
}

void Decoder::_bitDecode(const Matrix<BitCodeUnit>& bitCodeMatrix) {
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
		#ifdef TRACE_RLC
			printf("MCU(%d,%d)\n", r, c);
		#endif
			_bitDecodeOneMCU(bitCodeMatrix[r][c], (*_MCUs)[r][c]);
		}
	}
}

void Decoder::_rshift128(Block& block) {
	for (int r = 0; r < BLOCK_ROWCNT; ++r) {
		for (int c = 0; c < BLOCK_COLCNT; ++c) {
			block[r][c] += 128;
		}
	}
}

void Decoder::_makeYCbCrDataOfOneMCU(const MCU& mcu, int pos_x, int pos_y, Matrix<YCbCr>& ycbcrData) {
	int stride_r = _subsampFact.factor_v;
	int stride_c = _subsampFact.factor_h;
	int row_cnt = BLOCK_ROWCNT * stride_r;
	int col_cnt = BLOCK_COLCNT * stride_c;
	int imgbound_col = _width;
	int imgbound_row = _height;
	//写入Y
	for (int r = 0; r < row_cnt; ++r) {
		int imgpos_y = pos_y + r;
		int y_unitSel_r = r % BLOCK_ROWCNT;
		for (int c = 0; c < col_cnt; ++c) {
			int imgpos_x = pos_x + c;
			int y_blockSel = stride_c * (r / BLOCK_ROWCNT) + c / BLOCK_ROWCNT;
			int y_unitSel_c = c % BLOCK_COLCNT;
			assert((DWORD)imgpos_x < ycbcrData.column_cnt && (DWORD)imgpos_y < ycbcrData.row_cnt);
			ycbcrData[imgpos_y][imgpos_x].Y = mcu.y[y_blockSel][0][y_unitSel_r][y_unitSel_c];
		}
	}
	//写入CbCr
	for (int r = 0; r < row_cnt; ++r) {
		int imgpos_y = pos_y + r;
		for (int c = 0; c < col_cnt; ++c) {
			int imgpos_x = pos_x + c;
			assert((DWORD)imgpos_x < ycbcrData.column_cnt && (DWORD)imgpos_y < ycbcrData.row_cnt);
			ycbcrData[imgpos_y][imgpos_x].Cb = mcu.cb[0][r / stride_r][c / stride_c];
			ycbcrData[imgpos_y][imgpos_x].Cr = mcu.cr[0][r / stride_r][c / stride_c];
		}
	}
}

void Decoder::_makeYCbCrDataOfBoundaryMCU(const MCU& mcu, int pos_x, int pos_y, Matrix<YCbCr>& ycbcrData) {
	int stride_r = _subsampFact.factor_v;
	int stride_c = _subsampFact.factor_h;
	int row_cnt = BLOCK_ROWCNT * stride_r;
	int col_cnt = BLOCK_COLCNT * stride_c;
	int imgbound_col = _width;
	int imgbound_row = _height;

	//写入Y
	for (int r = 0; r + pos_y < imgbound_row && r < row_cnt; ++r) {
		int y_unitSel_r = r % BLOCK_ROWCNT;
		int imgpos_y = pos_y + r;
		for (int c = 0; c + pos_x < imgbound_col && c < col_cnt; ++c) {
			int imgpos_x = pos_x + c;
			int y_blockSel = stride_c * (r / BLOCK_ROWCNT) + c / BLOCK_ROWCNT;
			int y_unitSel_c = c % BLOCK_COLCNT;
			assert(imgpos_x < ycbcrData.column_cnt && imgpos_y < ycbcrData.row_cnt);
			ycbcrData[imgpos_y][imgpos_x].Y = mcu.y[y_blockSel][0][y_unitSel_r][y_unitSel_c];
		}
	}
	//写入CbCr
	for (int r = 0; r + pos_y < imgbound_row && r < row_cnt; ++r) {
		int imgpos_y = pos_y + r;
		for (int c = 0; c + pos_x < imgbound_col && c < col_cnt; ++c) {
			int imgpos_x = pos_x + c;
			assert(imgpos_x < ycbcrData.column_cnt && imgpos_y < ycbcrData.row_cnt);
			ycbcrData[imgpos_y][imgpos_x].Cb = mcu.cb[0][r / stride_r][c / stride_c];
			ycbcrData[imgpos_y][imgpos_x].Cr = mcu.cr[0][r / stride_r][c / stride_c];
		}
	}
}

void Decoder::_normalize(Block& block) {
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		for (int j = 0; j < BLOCK_COLCNT; ++j) {
			if (block[i][j] > 255) {
				block[i][j] = 255;
			} else if (block[i][j] < 0) {
				block[i][j] = 0;
			}
		}
	}
}

Decoder::Decoder() :_MCUs(nullptr) {
}

Decoder::~Decoder() {
	if (_MCUs) {
		_deallocMCUs();
	}
	delete _MCUs;
}

void Decoder::setCodedData(std::vector<BYTE>&& codedData) {
	_codedData = std::vector<BYTE>(std::move(codedData));
}

void Decoder::setQuantTable(const int id, const BYTE(&quantTable)[BLOCK_ROWCNT][BLOCK_COLCNT]) {
	if (_quantTables.size() <= id) {
		_quantTables.resize(id + 1);
	}
	_quantTables[id] = &quantTable;
}

void Decoder::setComponentConfigs(const ComponentConfig(&cmptCfgs)[(int)Component::MAXENUMVAL]) {
	_cmptCfgs[(int)Component::LUMA] = cmptCfgs[(int)Component::LUMA];
	_cmptCfgs[(int)Component::CHROMA_B] = cmptCfgs[(int)Component::CHROMA_B];
	_cmptCfgs[(int)Component::CHROMA_R] = cmptCfgs[(int)Component::CHROMA_R];
	_subsampFact = cmptCfgs[(int)Component::LUMA].subsampFact;
}

void Decoder::setSize(WORD width, WORD height) {
	_width = width;
	_height = height;
}

void Decoder::setSymbolTable(const int id, const HuffmanTable& huffTable, const HTableType type) {
	//一般情况下,id是连续的
	if (_symbolTables[(int)type].size() <= id) {
		_symbolTables[(int)type].resize(id + 1);
	#ifdef STAT_SYMBOL_FREQ
		_bitStringTables[(int)type].resize(id + 1);
	#endif
	}
	_symbolTables[(int)type][id] = SymbolTable(0xFFFF);

	std::vector<std::vector<BitString>> bitStringTable;
	IntHuffman::getCanonicalCodes(huffTable, bitStringTable);
	size_t len = huffTable.size();

	for (size_t i = 1; i < len; ++i) {
		size_t cnt = huffTable[i].size();
		for (size_t j = 0; j < cnt; ++j) {
			BYTE symbol = (BYTE)huffTable[i][j];
			int length = bitStringTable[i][j].length();
			_symbolTables[(int)type][id].symbol(bitStringTable[i][j]) = Symbol(symbol, length);
		}
	}

#ifdef STAT_SYMBOL_FREQ
	_bitStringTables[(int)type][id] = bitStringTable;
#endif
}

/*过程:
*
* 原始数据-->哈夫曼解码-->RLE解码-->还原每个Block(Z字形排列的)
* +----HuffmanDecode----++-----------BitDecode-----------+
*/
void Decoder::_decodeOneMCU(MCU& mcu, BitReader& reader) {
	BitCodeUnit bitCodeUnit;
	_huffmanDecodeOneMCU(bitCodeUnit, reader);
	_bitDecodeOneMCU(bitCodeUnit, mcu);
}

void Decoder::decode() {
	int mcus_col = (ALIGN(_width, _subsampFact.factor_h + 2)) / (_subsampFact.factor_h * BLOCK_COLCNT);
	int mcus_row = (ALIGN(_height, _subsampFact.factor_v + 2)) / (_subsampFact.factor_v * BLOCK_ROWCNT);
	_MCUs = new Matrix<MCU>(mcus_row, mcus_col);
	_initAllocMCUs();
	BitReader reader(_codedData);
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			#ifdef TRACE_MCU_RLC
				printf("MCU(%d,%d)\n", r, c);
			#endif
			_decodeOneMCU((*_MCUs)[r][c], reader);
		}
	}
#ifdef STAT_SYMBOL_FREQ
	_printSymbolTableStatistics();
#endif
}

//对Z字形排列的序列dequantize, 由于量化表也是Z字形排列的,因此在dequantize之后需要将其正过来
void Decoder::dequantize() {
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			MCU& mcu = (*_MCUs)[r][c];
			for (DWORD i = 0; i < _subsampFact.factor_v; ++i) {
				for (DWORD j = 0; j < _subsampFact.factor_h; ++j) {
					Block& input = *mcu.y[i * _subsampFact.factor_h + j];
					Block& output = input;
					Quantizer::dequantize(input, *_quantTables[_cmptCfgs[(int)Component::LUMA].QTableSel], output);
				}
			}
			Quantizer::dequantize(*mcu.cb, *_quantTables[_cmptCfgs[(int)Component::CHROMA_B].QTableSel], *mcu.cb);
			Quantizer::dequantize(*mcu.cr, *_quantTables[_cmptCfgs[(int)Component::CHROMA_R].QTableSel], *mcu.cr);
		}
	}
}

void Decoder::doIDCT() {
	for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
		for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
			//在MCU矩阵中选择一个MCU, 并分配新的MCU
		#ifdef TRACE_MCU_INDEX
			printf("MCU(%d,%d)\n", r, c);
		#endif
			MCU& mcu = (*_MCUs)[r][c];
			MCU newMCU;
			Block dezigzagged;
			_allocMCU(newMCU);
			//对Y块进行IFDCT
			for (DWORD i = 0; i < _subsampFact.factor_v; ++i) {
				for (DWORD j = 0; j < _subsampFact.factor_h; ++j) {
				#ifdef TRACE_BEFORE_IDCT
					printf("Zig[DCT[Y[%d][%d]]]\n", i, j);
					printBlock(*mcu.y[i * _subsampFact.factor_h + j]);
				#endif
					dezigzagSequence((int(&)[64]) * mcu.y[i * _subsampFact.factor_h + j], dezigzagged);
				#ifdef TRACE_BEFORE_IDCT
					printf("DCT[Y[%d][%d]]\n", i, j);
					printBlock(dezigzagged);
				#endif
					Block& output = *newMCU.y[i * _subsampFact.factor_h + j];
					DCT::inverseDCT(dezigzagged, output);
					_rshift128(output);
					_normalize(output);
				#ifdef TRACE_AFTER_IDCT
					printf("Y[%d][%d]\n", i, j);
					printBlock(output);
				#endif
				}
			}
			//对Cb,Cr块进行FDCT
		#ifdef TRACE_BEFORE_IDCT
			printf("Zig[DCT[Cb]]\n");
			printBlock(*mcu.cb);
		#endif
			dezigzagSequence((int(&)[64]) * mcu.cb, dezigzagged);
		#ifdef TRACE_BEFORE_IDCT
			printf("DCT[Cb]\n");
			printBlock(dezigzagged);
		#endif
			DCT::inverseDCT(dezigzagged, *newMCU.cb);
			_rshift128(*newMCU.cb);
			_normalize(*newMCU.cb);
		#ifdef TRACE_AFTER_IDCT
			printf("Cb\n");
			printBlock(*newMCU.cb);
		#endif

		#ifdef TRACE_BEFORE_IDCT
			printf("Zig[DCT[Cr]]\n");
			printBlock(*mcu.cr);
		#endif
			dezigzagSequence((int(&)[64]) * mcu.cr, dezigzagged);
		#ifdef TRACE_BEFORE_IDCT
			printf("DCT[Cr]\n");
			printBlock(dezigzagged);
		#endif
			DCT::inverseDCT(dezigzagged, *newMCU.cr);
			_rshift128(*newMCU.cr);
			_normalize(*newMCU.cr);
		#ifdef TRACE_AFTER_IDCT
			printf("Cr\n");
			printBlock(*newMCU.cr);
		#endif

			//更新到MCU矩阵
			_updateMCU(mcu, newMCU);
		}
	}
}

void Decoder::makeYCbCrData(Matrix<YCbCr>& ycbcrData) {
	DWORD r, c;
	int mcu_colUnitCnt = BLOCK_COLCNT * _subsampFact.factor_h;
	int mcu_rowUnitCnt = BLOCK_ROWCNT * _subsampFact.factor_v;
	for (r = 0; r < _MCUs->row_cnt - 1; ++r) {
		for (c = 0; c < _MCUs->column_cnt - 1; ++c) {
			_makeYCbCrDataOfOneMCU((*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt, ycbcrData);
		}
		_makeYCbCrDataOfBoundaryMCU((*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt, ycbcrData);
	}
	for (c = 0; c < _MCUs->column_cnt; ++c) {
		_makeYCbCrDataOfBoundaryMCU((*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt, ycbcrData);
	}
}
