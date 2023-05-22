#pragma once

#include <vector>
#include "typedef.h"
#include "BitString.h"
#include "CodingUtil.h"
#include "BitReader.h"
#include "IntHuffman.h"
#include "Quantizer.h"
#include "UtilFunc.h"
#include "DCT.h"

class Decoder
{
private:
	struct Symbol {
		BYTE symbol;
		int length;
	};

	class SymbolTable {
	private:
		//哈夫曼编码-符号映射表
		//符号不存在时,length==0
		Symbol* _symbols;
		size_t _count;
	public:
		SymbolTable() :
			_count(0),
			_symbols(nullptr) {
		}
		SymbolTable(size_t symbolCnt) :
			_count(symbolCnt),
			_symbols(new Symbol[symbolCnt]) {
			memset(_symbols, 0, _count * sizeof(Symbol));
		}
		~SymbolTable(){
			delete[] _symbols;
		}
		bool hasSymbol(const BitString& bitString) const {
			return _symbols[bitString.value()].length == bitString.length();
		}

		Symbol& symbol(const BitString& bitString) {
			return _symbols[bitString.value()];
		}

		const Symbol& symbol(const BitString& bitString) const {
			return _symbols[bitString.value()];
		}
	};
	//符号表(二维数组)
	//第一维:符号表适应类型(DC/AC)
	//第二维:符号表编号(与JPEG中的哈夫曼表编号相同)
	std::vector<SymbolTable> _symbolTable[(int)HTableType::MAXENUMVAL];
	std::vector<BYTE[8][8]> _quantTables;	//注意:JPG文件中的量化表是以Z字形顺序排列后的量化表(原封不动添加到_quantTables数组,故是Z字形排列的)
	int _quantTableSel_Y;
	int _quantTableSel_Cb;
	int _quantTableSel_Cr;
	int _tableSel_Y;
	int _tableSel_Cb;
	int _tableSel_Cr;
	Matrix<MCU>* _MCUs{};
	DPCM _dpcmEncoders[3];
	WORD _width{};
	WORD _height{};
	SubsampFact _subsampFact{};
	std::vector<BYTE> _codedData;

	//分配一块MCU
	void _allocMCU(MCU& mcu) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		mcu.y = new Block * [stride_r * stride_c];
		for (int i = 0; i < stride_r * stride_c; ++i) {
			mcu.y[i] = new Block[1];
		}
		mcu.cb = new Block[1];
		mcu.cr = new Block[1];
	}

	void _deallocMCU(MCU& mcu){
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
	void _updateMCU(MCU& oldMCU, MCU& newMCU) {
		MCU& mcu = oldMCU;
		_deallocMCU(oldMCU);
		mcu.y = newMCU.y;
		mcu.cb = newMCU.cb;
		mcu.cr = newMCU.cr;
	}

	void _initAllocMCUs() {
		for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
			for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
				_allocMCU((*_MCUs)[r][c]);
			}
		}
	}
	void _matchSymbol(const SymbolTable& table, BitReader& reader, Symbol& symbol) {
		BitString bs;
		do {
			bs.push_back(reader.readBit());
			if (bs.length() > HUFFMAN_CODE_LENGTH_LIMIT) {
				fprintf(stderr, "Cannot decode bitstring at data segment %#x.%d\n", reader.bytePosition(), reader.bitPosition());
				exit(1);
			}
		} while (table.hasSymbol(bs));
		symbol = table.symbol(bs);
	}
	void _huffmanDecodeOneBlock(const int tableID, BitCodeArray& bitCodes, BitReader& reader) {
		int count = 0;
		bool readEOB = false;
		Symbol symbol;
		BitCode bitCode;
		_matchSymbol(_symbolTable[(int)HTableType::DC][tableID], reader, symbol);
		bitCode.codedUnit = symbol.symbol;
		bitCode.bits = reader.readBits(bitCode.bitLength).value();
		bitCodes.push_back(bitCode);
		++count;
		while (count < 64 && !readEOB) {
			_matchSymbol(_symbolTable[(int)HTableType::AC][tableID], reader, symbol);
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

	void _huffmanDecodeOneMCU(BitCodeUnit& bitCodeUnit, BitReader& reader) {
		int i, j;
		BitCodeArray bitCodes;
		for (i = 0; i < _subsampFact.factor_v; ++i) {
			for (j = 0; j < _subsampFact.factor_h; ++j) {
				_huffmanDecodeOneBlock(_tableSel_Y, bitCodes, reader);
				bitCodeUnit.y.push_back(bitCodes);
			}
		}
		_huffmanDecodeOneBlock(_tableSel_Cb, bitCodes, reader);
		bitCodeUnit.cb = bitCodes;
		_huffmanDecodeOneBlock(_tableSel_Cr, bitCodes, reader);
		bitCodeUnit.cr = bitCodes;
	}

	void _huffmanDecode(Matrix<BitCodeUnit>& bitCodeMatrix) {
		BitReader reader(_codedData);
		for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
			for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
				_huffmanDecodeOneMCU(bitCodeMatrix[r][c], reader);
			}
		}
	}

	void _bitDecodeOneBlock(const BitCodeArray& bitCodes, Block& block) {
		int count = bitCodes.size();
		//不可能超过256,否则jpg文件本身就有问题
		RLCode codes[256];
		for (int i = 0; i < count; ++i) {
			codes[i].value = BitCodec::getValue(BitString(bitCodes[i].bits, bitCodes[i].bitLength));
			codes[i].zeroCnt = bitCodes[i].zeroCnt;
		}
		RunLengthCodec::decode(codes, count, 64, (int*)block);
	}

	void _bitDecodeOneMCU(const BitCodeUnit& bitCodeUnit, MCU& mcu) {
		int i, j;
		BitCodeArray bitCodes;
		for (i = 0; i < _subsampFact.factor_v; ++i) {
			for (j = 0; j < _subsampFact.factor_h; ++j) {
				_bitDecodeOneBlock(bitCodeUnit.y[i * _subsampFact.factor_h + j], *mcu.y[i * _subsampFact.factor_h + j]);
			}
		}
		_bitDecodeOneBlock(bitCodeUnit.cb, *mcu.cb);
		_bitDecodeOneBlock(bitCodeUnit.cr, *mcu.cr);
	}

	void _bitDecode(const Matrix<BitCodeUnit>& bitCodeMatrix) {
		for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
			for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
				_bitDecodeOneMCU(bitCodeMatrix[r][c], (*_MCUs)[r][c]);
			}
		}
	}

	void _rshift128(Block& block) {
		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				block[r][c] += 128;
			}
		}
	}

public:
	void setCodedData(const std::vector<BYTE>& codedData) {
		_codedData = codedData;
	}
	void setSubsamplingFactor(SubsampFact& subsampFact) {
		_subsampFact = subsampFact;
	}
	void setSize(WORD width,WORD height) {
		_width = width;
		_height = height;
		int mcus_col = (ALIGN(_width, _subsampFact.factor_h + 2)) / (_subsampFact.factor_h * BLOCK_COLCNT);
		int mcus_row = (ALIGN(_height, _subsampFact.factor_v + 2)) / (_subsampFact.factor_v * BLOCK_ROWCNT);
		_MCUs = new Matrix<MCU>(mcus_row, mcus_col);
	}
	void setSymbolTable(const HuffmanTable& huffTable,const int id,const HTableType type) {
		//一般情况下,id是连续的
		while (_symbolTable[(int)type].size() <= id) {
			_symbolTable[(int)type].push_back(SymbolTable());
		}
		_symbolTable[(int)type][id] = SymbolTable(0xFFFF);
		std::vector<std::vector<BitString>> bitStringTable;
		IntHuffman::getCanonicalCodes(huffTable, bitStringTable);
		size_t len = huffTable.size();
		for (size_t i = 1; i < len; ++i) {
			size_t cnt = huffTable[i].size();
			for (size_t j = 0; j < cnt;++j) {
				_symbolTable[(int)type][id].symbol(bitStringTable[i][j]) = { (BYTE)huffTable[i][j] ,bitStringTable[i][j].length()};
			}
		}
	}

	//过程:
	//
	// 原始数据-->哈夫曼解码-->RLE解码-->还原每个Block(Z字形排列的)
	// +--HuffmanDecode--++------BitDecode--------+
	//
	void decode() {
		Matrix<BitCodeUnit> bitCodeMatrix(_MCUs->row_cnt, _MCUs->column_cnt);
		_huffmanDecode(bitCodeMatrix);
		_bitDecode(bitCodeMatrix);
	}

	//对Z字形排列的序列dequantize, 由于量化表也是Z字形排列的,因此在dequantize之后再将其正过来
	void dequantize() {
		for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
			for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
				MCU& mcu = (*_MCUs)[r][c];
				for (DWORD i = 0; i < _subsampFact.factor_v; ++i) {
					for (DWORD j = 0; j < _subsampFact.factor_h; ++j) {
						Block& input = *mcu.y[i * _subsampFact.factor_h + j];
						Block& output = input;
						Quantizer::dequantize(input, _quantTables[_quantTableSel_Y], output);
					}
				}
				Quantizer::dequantize(*mcu.cb, _quantTables[_quantTableSel_Cb], *mcu.cb);
				Quantizer::dequantize(*mcu.cr, _quantTables[_quantTableSel_Cr], *mcu.cr);
			}
		}
	}

	void doIDCT() {
		for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
			for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
				//在MCU矩阵中选择一个MCU, 并分配新的MCU
				MCU& mcu = (*_MCUs)[r][c];
				MCU newMCU;
				Block zigzagged;
				_allocMCU(newMCU);
				//对Y块进行IFDCT
				for (DWORD i = 0; i < _subsampFact.factor_v; ++i) {
					for (DWORD j = 0; j < _subsampFact.factor_h; ++j) {
						getZigzaggedSequence(*mcu.y[i * _subsampFact.factor_h + j], (int(&)[64])zigzagged);
						_rshift128(zigzagged);
						Block& output = *newMCU.y[i * _subsampFact.factor_h + j];
						DCT::inverseDCT(zigzagged, output);
					}
				}
				//对Cb,Cr块进行FDCT
				getZigzaggedSequence(*mcu.cb, (int(&)[64])zigzagged);
				_rshift128(zigzagged);
				DCT::inverseDCT(zigzagged, *newMCU.cb);
				getZigzaggedSequence(*mcu.cr, (int(&)[64])zigzagged);
				_rshift128(zigzagged);
				DCT::inverseDCT(zigzagged, *newMCU.cr);
				//更新到MCU矩阵
				_updateMCU(mcu, newMCU);
			}
		}
	}

	void _makeYCbCrDataOfOneMCU(const MCU& mcu, int pos_x, int pos_y, Matrix<YCbCr>& ycbcrData) {
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
				ycbcrData[imgpos_y][imgpos_x].Y = mcu.y[y_blockSel][0][y_unitSel_r][y_unitSel_c];
			}
		}
		//写入CbCr
		for (int r = 0; r < row_cnt; ++r) {
			int imgpos_y = pos_y + r;
			for (int c = 0; c < col_cnt; ++c) {
				int imgpos_x = pos_x + c;
				ycbcrData[imgpos_y][imgpos_x].Cb = mcu.cb[0][r / stride_r][c / stride_c];
				ycbcrData[imgpos_y][imgpos_x].Cr = mcu.cr[0][r / stride_r][c / stride_c];
			}
		}
	}

	void _makeYCbCrDataOfBoundaryMCU(const MCU& mcu, int pos_x, int pos_y, Matrix<YCbCr>& ycbcrData) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		int row_cnt = BLOCK_ROWCNT * stride_r;
		int col_cnt = BLOCK_COLCNT * stride_c;
		int imgbound_col = _width;
		int imgbound_row = _height;

		//写入Y
		for (int r = 0; r + pos_y  < imgbound_row; ++r) {
			int y_unitSel_r = r % BLOCK_ROWCNT;
			int imgpos_y = pos_y + r;
			for (int c = 0; c + pos_x  < imgbound_col; ++c) {
				int imgpos_x = pos_x + c; 
				int y_blockSel = stride_c * (r / BLOCK_ROWCNT) + c / BLOCK_ROWCNT;
				int y_unitSel_c = c % BLOCK_COLCNT;
				ycbcrData[imgpos_y][imgpos_x].Y = mcu.y[y_blockSel][0][y_unitSel_r][y_unitSel_c];
			}
		}
		//写入CbCr
		for (int r = 0; r + pos_y < imgbound_row; ++r) {
			int imgpos_y = pos_y + r;
			for (int c = 0; c + pos_y < imgbound_col; ++c) {
				int imgpos_x = pos_x + c;
				ycbcrData[imgpos_y][imgpos_x].Cb = mcu.cb[0][r / stride_r][c / stride_c];
				ycbcrData[imgpos_y][imgpos_x].Cr = mcu.cr[0][r / stride_r][c / stride_c];
			}
		}
	}

	void makeYCbCrData(Matrix<YCbCr>& ycbcrData) {
		int r, c;
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

};

