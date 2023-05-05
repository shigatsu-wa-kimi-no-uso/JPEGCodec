#pragma once
/*
* CodingUnits.h
* Written by kiminouso, 2023/05/01
*/
#ifndef CodingUnits_h__
#define CodingUnits_h__
#include "typedef.h"
#include "Quantizer.h"
#include "CodingUtil.h"
#include "DCT.h"

class CodingUnits {
private:
	struct BitCode {
		union {
			BYTE bitLength : 4;
			BYTE zeroCnt : 4;
			BYTE codedUnit;
		};
		DWORD bits;
	};
	SubsampFact _subsampFact;
	Matrix<MCU>* _MCUs;
	void _makeBoundaryMCU(Matrix<YCbCr>* ycbcrData, MCU* mcu, int pos_x, int pos_y) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		int row_cnt = BLOCK_ROWCNT * stride_r;
		int col_cnt = BLOCK_COLCNT * stride_c;
		int imgbound_col = ycbcrData->column_cnt;
		int imgbound_row = ycbcrData->row_cnt;
		mcu->y = new Block * [stride_r * stride_c];
		for (int i = 0; i < stride_r * stride_c; ++i) {
			mcu->y[i] = new Block[1];
		}
		mcu->cb = new Block[1];
		mcu->cr = new Block[1];

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

	void _makeOneMCU(Matrix<YCbCr>* ycbcrData, MCU* mcu, int pos_x, int pos_y) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		int row_cnt = BLOCK_ROWCNT * stride_r;
		int col_cnt = BLOCK_COLCNT * stride_c;
		mcu->y = new Block*[stride_r * stride_c];
		for (int i = 0; i < stride_r * stride_c; ++i) {
			mcu->y[i] = new Block[1];
		}
		mcu->cb = new Block[1];
		mcu->cr = new Block[1];

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

	void _updateMCU(MCU* newMCU, int r, int c) {
		MCU* mcu = &(*_MCUs)[r][c];
		for (int i = 0; i < _subsampFact.factor_v; ++i) {
			for (int j = 0; j < _subsampFact.factor_h; ++j) {
				delete[] mcu->y[i * _subsampFact.factor_h + j];
				mcu->y[i * _subsampFact.factor_h + j] = newMCU->y[i * _subsampFact.factor_h + j];
			}
		}
		delete[] mcu->cb;
		delete[] mcu->cr;
		mcu->cb = newMCU->cb;
		mcu->cr = newMCU->cr;
	}

	void _lshift128(Block* block) {
		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				(*block)[r][c] -= 128;
			}
		}
	}

	void _bitEncodeOneBlock(Block* input, BitCode* outputBuf) {
		RLECode RLEcodeBuf[256];
		int DCDiff = DPCM::nextDiff((*input)[0][0]);
		RLEcodeBuf[0].zeroCnt = 0;
		RLEcodeBuf[0].value = DCDiff;
		int count;
		RLE::getRLECodes(input, &RLEcodeBuf[1], &count);
		count++;
		for (int i = 0; i < count; ++i) {
			BitString bitString = BitEncoder::getBitString(RLEcodeBuf[i].value);
			outputBuf[i].zeroCnt = RLEcodeBuf[i].zeroCnt;
			outputBuf[i].bits = bitString.value();
			outputBuf[i].bitLength = bitString.length();
		}
	}
public:
	CodingUnits() {}
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
		int mcus_col = ALIGN(ycbcrData->column_cnt, _subsampFact.factor_h + 2) / (_subsampFact.factor_h * BLOCK_COLCNT);
		int mcus_row = ALIGN(ycbcrData->row_cnt, _subsampFact.factor_v + 2) / (_subsampFact.factor_v * BLOCK_ROWCNT);
		int mcu_colUnitCnt = BLOCK_COLCNT * subsampFact.factor_h;
		int mcu_rowUnitCnt = BLOCK_ROWCNT * subsampFact.factor_v;
		_MCUs = new Matrix<MCU>(mcus_row, mcus_col);
		int r,c;
		for (r = 0; r < mcus_row - 1; ++r) {
			for (c = 0; c < mcus_col - 1; ++c) {
				_makeOneMCU(ycbcrData, &(*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
			}
			_makeBoundaryMCU(ycbcrData, &(*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
		}
		for (c = 0; c < mcus_col; ++c) {
			_makeBoundaryMCU(ycbcrData, &(*_MCUs)[r][c], c * mcu_colUnitCnt, r * mcu_rowUnitCnt);
		}
	}

	void doFDCT() {
		for (int r = 0; _MCUs->row_cnt; ++r) {
			for (int c = 0; _MCUs->column_cnt; ++c) {
				MCU* mcu = _MCUs[r][c];
				MCU* newMCU = new MCU;
				Block* input;
				Block* output;
				for (int i = 0; i < _subsampFact.factor_v; ++i) {
					for (int j = 0; j < _subsampFact.factor_h; ++j) {
						input = mcu->y[i * _subsampFact.factor_h + j];
						_lshift128(input);
						output = new Block[1];
						DCT::transform(input, output);
						newMCU->y[i * _subsampFact.factor_h + j] = output;
					}
				}
				input = mcu->cb;
				_lshift128(input);
				output = new Block[1];
				DCT::transform(input, output);
				newMCU->cb = output;
				input = mcu->cr;
				_lshift128(input);
				output = new Block[1];
				DCT::transform(input, output);
				newMCU->cr = output;
				_updateMCU(newMCU, r, c);
			}
		}
	}

	void doQuantize() {
		for (int r = 0; _MCUs->row_cnt; ++r) {
			for (int c = 0; _MCUs->column_cnt; ++c) {
				MCU* mcu = _MCUs[r][c];
				Block* input;
				Block* output;
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



	void _bitEncode(BitCode**** bitCodeBuf) {
		for (int r = 0; _MCUs->row_cnt; ++r) {
			for (int c = 0; _MCUs->column_cnt; ++c) {
				MCU* mcu = _MCUs[r][c];
				Block* input;
				Block* output;
				int i,j;
				for (i = 0; i < _subsampFact.factor_v; ++i) {
					for (j = 0; j < _subsampFact.factor_h; ++j) {
						input = mcu->y[i * _subsampFact.factor_h + j];
						_bitEncodeOneBlock(input, bitCodeBuf[r][c][i * _subsampFact.factor_h + j]);
					}
				}
				input = mcu->cb;
				output = input;
				_bitEncodeOneBlock(input, bitCodeBuf[r][c][i * j]);
				input = mcu->cr;
				output = input;
				_bitEncodeOneBlock(input, bitCodeBuf[r][c][i * j + 1]);
			}
		}
	}

	void huffmanEncode() {

	}

	Matrix<MCU>* getMCUs() {
		return _MCUs;
	}
};
#endif // CodingUnits_h__