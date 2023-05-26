/*
* Decoder.h
* Written by kiminouso, 2023/05/26
*/
#pragma once
#ifndef Decoder_h__
#define Decoder_h__
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
		//����������-����ӳ���
		//���Ų�����ʱ,length==0
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
	//���ű�(��ά����)
	//��һά:���ű���Ӧ����(DC/AC)
	//�ڶ�ά:���ű���(��JPEG�еĹ�����������ͬ)
	std::vector<SymbolTable> _symbolTables[(int)HTableType::MAXENUMVAL];
	std::vector<BYTE[8][8]> _quantTables;	//ע��:JPG�ļ��е�����������Z����˳�����к��������(ԭ�ⲻ����ӵ�_quantTables����,����Z�������е�)
	ComponentConfig _cmptCfgs[(int)Component::MAXENUMVAL];
	Matrix<MCU>* _MCUs{};
	DPCM _dpcmEncoders[3];
	WORD _width{};
	WORD _height{};
	SubsampFact _subsampFact{};
	std::vector<BYTE> _codedData;

	//����һ��MCU
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

	//ע��:����MCU����ʱ����ǳ����
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

	void _huffmanDecodeOneBlock(const int tableID_DC, const int tableID_AC, BitCodeArray& bitCodes, BitReader& reader) {
		int count = 0;
		bool readEOB = false;
		Symbol symbol;
		BitCode bitCode;
		_matchSymbol(_symbolTables[(int)HTableType::DC][tableID_DC], reader, symbol);
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

	void _huffmanDecodeOneMCU(BitCodeUnit& bitCodeUnit, BitReader& reader) {
		int i, j;
		BitCodeArray bitCodes;
		for (i = 0; i < _subsampFact.factor_v; ++i) {
			for (j = 0; j < _subsampFact.factor_h; ++j) {
				_huffmanDecodeOneBlock(
					_cmptCfgs[(int)Component::LUMA].DC_HTableSel,
					_cmptCfgs[(int)Component::LUMA].AC_HTableSel,
					bitCodes,
					reader);
				bitCodeUnit.y.push_back(bitCodes);
			}
		}
		_huffmanDecodeOneBlock(
			_cmptCfgs[(int)Component::CHROMA_B].DC_HTableSel,
			_cmptCfgs[(int)Component::CHROMA_B].AC_HTableSel,
			bitCodes, 
			reader);
		bitCodeUnit.cb = bitCodes;
		_huffmanDecodeOneBlock(
			_cmptCfgs[(int)Component::CHROMA_R].DC_HTableSel,
			_cmptCfgs[(int)Component::CHROMA_R].AC_HTableSel,
			bitCodes,
			reader);
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
		//�����ܳ���256,����jpg�ļ������������
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

	void _makeYCbCrDataOfOneMCU(const MCU& mcu, int pos_x, int pos_y, Matrix<YCbCr>& ycbcrData) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		int row_cnt = BLOCK_ROWCNT * stride_r;
		int col_cnt = BLOCK_COLCNT * stride_c;
		//д��Y
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
		//д��CbCr
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

		//д��Y
		for (int r = 0; r + pos_y < imgbound_row; ++r) {
			int y_unitSel_r = r % BLOCK_ROWCNT;
			int imgpos_y = pos_y + r;
			for (int c = 0; c + pos_x < imgbound_col; ++c) {
				int imgpos_x = pos_x + c;
				int y_blockSel = stride_c * (r / BLOCK_ROWCNT) + c / BLOCK_ROWCNT;
				int y_unitSel_c = c % BLOCK_COLCNT;
				ycbcrData[imgpos_y][imgpos_x].Y = mcu.y[y_blockSel][0][y_unitSel_r][y_unitSel_c];
			}
		}
		//д��CbCr
		for (int r = 0; r + pos_y < imgbound_row; ++r) {
			int imgpos_y = pos_y + r;
			for (int c = 0; c + pos_y < imgbound_col; ++c) {
				int imgpos_x = pos_x + c;
				ycbcrData[imgpos_y][imgpos_x].Cb = mcu.cb[0][r / stride_r][c / stride_c];
				ycbcrData[imgpos_y][imgpos_x].Cr = mcu.cr[0][r / stride_r][c / stride_c];
			}
		}
	}

public:
	void setCodedData(std::vector<BYTE>&& codedData) {
		_codedData = std::vector<BYTE>(std::move(codedData));
	}

	void setComponentConfigs(const ComponentConfig(&cmptCfgs)[(int)Component::MAXENUMVAL]) {
		_cmptCfgs[(int)Component::LUMA] = cmptCfgs[(int)Component::LUMA];
		_cmptCfgs[(int)Component::CHROMA_B] = cmptCfgs[(int)Component::CHROMA_B];
		_cmptCfgs[(int)Component::CHROMA_R] = cmptCfgs[(int)Component::CHROMA_R];
		_subsampFact = cmptCfgs[(int)Component::LUMA].subsampFact;
	}

	void setSize(WORD width,WORD height) {
		_width = width;
		_height = height;
		int mcus_col = (ALIGN(_width, _subsampFact.factor_h + 2)) / (_subsampFact.factor_h * BLOCK_COLCNT);
		int mcus_row = (ALIGN(_height, _subsampFact.factor_v + 2)) / (_subsampFact.factor_v * BLOCK_ROWCNT);
		_MCUs = new Matrix<MCU>(mcus_row, mcus_col);
	}

	void setSymbolTable(const int id,const HuffmanTable& huffTable,const HTableType type) {
		//һ�������,id��������
		while (_symbolTables[(int)type].size() <= id) {
			_symbolTables[(int)type].push_back(SymbolTable());
		}
		_symbolTables[(int)type][id] = SymbolTable(0xFFFF);
		std::vector<std::vector<BitString>> bitStringTable;
		IntHuffman::getCanonicalCodes(huffTable, bitStringTable);
		size_t len = huffTable.size();
		for (size_t i = 1; i < len; ++i) {
			size_t cnt = huffTable[i].size();
			for (size_t j = 0; j < cnt;++j) {
				_symbolTables[(int)type][id].symbol(bitStringTable[i][j]) = { (BYTE)huffTable[i][j] ,bitStringTable[i][j].length()};
			}
		}
	}

	//����:
	//
	// ԭʼ����-->����������-->RLE����-->��ԭÿ��Block(Z�������е�)
	// +--HuffmanDecode--++------BitDecode--------+
	//
	void decode() {
		Matrix<BitCodeUnit> bitCodeMatrix(_MCUs->row_cnt, _MCUs->column_cnt);
		_huffmanDecode(bitCodeMatrix);
		_bitDecode(bitCodeMatrix);
	}

	//��Z�������е�����dequantize, ����������Ҳ��Z�������е�,�����dequantize֮���ٽ���������
	void dequantize() {
		for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
			for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
				MCU& mcu = (*_MCUs)[r][c];
				for (DWORD i = 0; i < _subsampFact.factor_v; ++i) {
					for (DWORD j = 0; j < _subsampFact.factor_h; ++j) {
						Block& input = *mcu.y[i * _subsampFact.factor_h + j];
						Block& output = input;
						Quantizer::dequantize(input, _quantTables[_cmptCfgs[(int)Component::LUMA].QTableSel], output);
					}
				}
				Quantizer::dequantize(*mcu.cb, _quantTables[_cmptCfgs[(int)Component::CHROMA_B].QTableSel], *mcu.cb);
				Quantizer::dequantize(*mcu.cr, _quantTables[_cmptCfgs[(int)Component::CHROMA_R].QTableSel], *mcu.cr);
			}
		}
	}

	void doIDCT() {
		for (DWORD r = 0; r < _MCUs->row_cnt; ++r) {
			for (DWORD c = 0; c < _MCUs->column_cnt; ++c) {
				//��MCU������ѡ��һ��MCU, �������µ�MCU
				MCU& mcu = (*_MCUs)[r][c];
				MCU newMCU;
				Block zigzagged;
				_allocMCU(newMCU);
				//��Y�����IFDCT
				for (DWORD i = 0; i < _subsampFact.factor_v; ++i) {
					for (DWORD j = 0; j < _subsampFact.factor_h; ++j) {
						getZigzaggedSequence(*mcu.y[i * _subsampFact.factor_h + j], (int(&)[64])zigzagged);
						_rshift128(zigzagged);
						Block& output = *newMCU.y[i * _subsampFact.factor_h + j];
						DCT::inverseDCT(zigzagged, output);
					}
				}
				//��Cb,Cr�����FDCT
				getZigzaggedSequence(*mcu.cb, (int(&)[64])zigzagged);
				_rshift128(zigzagged);
				DCT::inverseDCT(zigzagged, *newMCU.cb);
				getZigzaggedSequence(*mcu.cr, (int(&)[64])zigzagged);
				_rshift128(zigzagged);
				DCT::inverseDCT(zigzagged, *newMCU.cr);
				//���µ�MCU����
				_updateMCU(mcu, newMCU);
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


#endif // Decoder_h__
