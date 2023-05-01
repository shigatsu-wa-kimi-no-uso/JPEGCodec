#pragma once
/*
* CodingUnits.h
* Written by kiminouso, 2023/05/01
*/
#ifndef CodingUnits_h__
#define CodingUnits_h__
#include "typedef.h"

class CodingUnits {
private:
	SubsampFact _subsampFact;
	Matrix<MCU>* _MCUs;
	void _makeBoundaryMCU(Matrix<YCbCr>* ycbcrData, MCU* mcu, int pos_x, int pos_y) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		int row_cnt = BLOCK_ROWCNT * stride_r;
		int col_cnt = BLOCK_COLCNT * stride_c;
		int imgbound_col = ycbcrData->column_cnt;
		int imgbound_row = ycbcrData->row_cnt;
		mcu->y = new Block * [stride_r];
		for (int i = 0; i < stride_r; ++i) {
			mcu->y[i] = new Block[stride_c];
		}
		mcu->cb = new Block[1];
		mcu->cr = new Block[1];

		//写入Y
		for (int r = 0; r < row_cnt; ++r) {
			for (int c = 0; c < col_cnt; ++c) {
				int imgpos_y = pos_y + r >= imgbound_col ? imgbound_col - 1 : pos_y + r;
				int imgpos_x = pos_x + c >= imgbound_row ? imgbound_row - 1 : pos_x + c;
				int y_blockSel_r = r / BLOCK_ROWCNT;
				int y_blockSel_c = c / BLOCK_COLCNT;
				int y_unitSel_r = r % BLOCK_ROWCNT;
				int y_unitSel_c = c % BLOCK_COLCNT;
				mcu->y[y_blockSel_r][y_blockSel_c][y_unitSel_c][y_unitSel_c] = ycbcrData[0][imgpos_y][imgpos_x].Y;
			}
		}
		//写入CbCr
		for (int r = 0; r < row_cnt; r += stride_r) {
			for (int c = 0; c < col_cnt; c += stride_c) {
				int imgpos_y = pos_y + r >= imgbound_col ? imgbound_col - 1 : pos_y + r;
				int imgpos_x = pos_x + c >= imgbound_row ? imgbound_row - 1 : pos_x + c;
				mcu->cb[0][r / stride_r][c / stride_c] = ycbcrData[0][imgpos_y][imgpos_x].Cb;
				mcu->cr[0][r / stride_r][c / stride_c] = ycbcrData[0][imgpos_y][imgpos_x].Cr;
			}
		}
	}

	void _makeOneMCU(Matrix<YCbCr>* ycbcrData, MCU* mcu, int pos_x, int pos_y) {
		int stride_r = _subsampFact.factor_v;
		int stride_c = _subsampFact.factor_h;
		int row_cnt = BLOCK_ROWCNT * stride_r;
		int col_cnt = BLOCK_COLCNT * stride_c;
		mcu->y = new Block * [stride_r];
		for (int i = 0; i < stride_r; ++i) {
			mcu->y[i] = new Block[stride_c];
		}
		mcu->cb = new Block[1];
		mcu->cr = new Block[1];

		//写入Y
		for (int r = 0; r < row_cnt; ++r) {
			for (int c = 0; c < col_cnt; ++c) {
				int imgpos_y = pos_y + r;
				int imgpos_x = pos_x + c;
				int y_blockSel_r = r / BLOCK_ROWCNT;
				int y_blockSel_c = c / BLOCK_COLCNT;
				int y_unitSel_r = r % BLOCK_ROWCNT;
				int y_unitSel_c = c % BLOCK_COLCNT;
				mcu->y[y_blockSel_r][y_blockSel_c][y_unitSel_c][y_unitSel_c] = ycbcrData[0][imgpos_y][imgpos_x].Y;
			}
		}
		//写入CbCr
		for (int r = 0; r < row_cnt; r+=stride_r) {
			for (int c = 0; c < col_cnt; c+=stride_c) {
				int imgpos_y = pos_y + r;
				int imgpos_x = pos_x + c;
				mcu->cb[0][r / stride_r][c / stride_c] = ycbcrData[0][imgpos_y][imgpos_x].Cb;
				mcu->cr[0][r / stride_r][c / stride_c] = ycbcrData[0][imgpos_y][imgpos_x].Cr;
			}
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

	Matrix<MCU>* getMCUs() {
		return _MCUs;
	}
};
#endif // CodingUnits_h__