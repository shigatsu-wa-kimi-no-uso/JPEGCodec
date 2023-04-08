#include "DCTBlockSplitter.h"
#include<stdio.h>

void DCTBlockSplitter::setYCmptMatrix(Matrix<float>* matrix_y)
{
	this->matrix_y = matrix_y;
	blockcnt_y_h = matrix_y->column_cnt / dctblock_colcnt;	//分量矩阵的水平方向上可以分为多少DCT块(不包含不能被完整地分为8X8块的部分)
	blockcnt_y_v = matrix_y->row_cnt / dctblock_rowcnt;	//分量矩阵的竖直方向上可以分为多少DCT块
	matrix_y_extracol_cnt = matrix_y->column_cnt % dctblock_colcnt;	//检测8X8分块后是否有没覆盖到块
	matrix_y_extrarow_cnt = matrix_y->row_cnt % dctblock_rowcnt;	//并记录分量矩阵需要补充的行数和列数
}

void DCTBlockSplitter::setCbCmptMatrix(Matrix<float>* matrix_cb)
{
	this->matrix_cb = matrix_cb;
	blockcnt_cb_h = matrix_cb->column_cnt / dctblock_colcnt; //分量矩阵的水平方向上可以分为多少DCT块(不包含不能被完整地分为8X8块的部分)
	blockcnt_cb_v = matrix_cb->row_cnt / dctblock_rowcnt; //分量矩阵的竖直方向上可以分为多少DCT块
	matrix_cb_extracol_cnt = matrix_cb->column_cnt % dctblock_colcnt;	 //检测8X8分块后是否有没覆盖到块
	matrix_cb_extrarow_cnt = matrix_cb->row_cnt % dctblock_rowcnt;	 //并记录分量矩阵需要补充的行数和列数
}

void DCTBlockSplitter::setCrCmptMatrix(Matrix<float>* matrix_cr)
{
	this->matrix_cr = matrix_cr;
	blockcnt_cr_h = matrix_cr->column_cnt / dctblock_colcnt; //分量矩阵的水平方向上可以分为多少DCT块(不包含不能被完整地分为8X8块的部分)
	blockcnt_cr_v = matrix_cr->row_cnt / dctblock_rowcnt; //分量矩阵的竖直方向上可以分为多少DCT块
	matrix_cr_extracol_cnt = matrix_cr->column_cnt % dctblock_colcnt;	 //检测8X8分块后是否有没覆盖到块
	matrix_cr_extrarow_cnt = matrix_cr->row_cnt % dctblock_rowcnt;	 //并记录分量矩阵需要补充的行数和列数
}

DWORD DCTBlockSplitter::getYBlockCount()
{
	return (blockcnt_y_h + (matrix_y_extracol_cnt == 0 ? 0 : 1)) * (blockcnt_y_v + (matrix_y_extrarow_cnt == 0 ? 0 : 1));
}

DWORD DCTBlockSplitter::getCbBlockCount()
{
	return (blockcnt_cb_h + (matrix_cb_extracol_cnt == 0 ? 0 : 1)) * (blockcnt_cb_v + (matrix_cb_extrarow_cnt == 0 ? 0 : 1));
}

DWORD DCTBlockSplitter::getCrBlockCount()
{
	return (blockcnt_cr_h + (matrix_cr_extracol_cnt == 0 ? 0 : 1)) * (blockcnt_cr_v + (matrix_cr_extrarow_cnt == 0 ? 0 : 1));
}

void DCTBlockSplitter::_split(const Matrix<float>* matrix,const DWORD blockcnt_h,const DWORD blockcnt_v,const DWORD matrix_extracol_cnt,const DWORD matrix_extrarow_cnt, Matrix<float>* dctblock)
{
	DWORD blockRSel = 0, blockCSel = 0, matpos_r, matpos_c, blockIndex = 0;
	int r, c;
	for (blockRSel = 0; blockRSel < blockcnt_v; ++blockRSel) {
		for (blockCSel = 0; blockCSel < blockcnt_h; ++blockCSel) {
			for (r = 0; r < dctblock_rowcnt; ++r) {
				matpos_r = r + blockRSel * dctblock_rowcnt;
				for (c = 0; c < dctblock_colcnt; ++c) {		
					matpos_c = c + blockCSel * dctblock_colcnt;
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r,matpos_c, dctblock[blockIndex][r][c]);
				}
				printf("\n");
			}
			++blockIndex;
		}
		//处理最右边要补齐的块
		if (matrix_extracol_cnt != 0) {
			for (r = 0; r < dctblock_rowcnt; ++r) {
				matpos_r = r + blockRSel * dctblock_rowcnt;
				for (c = 0; c < matrix_extracol_cnt; ++c) {
					matpos_c = c + blockCSel * dctblock_colcnt;
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r, matpos_c, dctblock[blockIndex][r][c]);
				}
				for (; c < dctblock_colcnt; ++c) {
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r, matpos_c, dctblock[blockIndex][r][c]);
				}
				printf("\n");
			}
			++blockIndex;
		}

	}
	//处理最下边要补齐的块
	if (matrix_extrarow_cnt != 0) {
		for (blockCSel = 0; blockCSel < blockcnt_h; ++blockCSel) {
			for (r = 0; r < matrix_extrarow_cnt; ++r) {
				matpos_r = r + blockRSel * dctblock_rowcnt;
				for (c = 0; c < dctblock_colcnt; ++c) {
					matpos_c = c + blockCSel * dctblock_colcnt;
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r, matpos_c, dctblock[blockIndex][r][c]);
				}
				printf("\n");
			}
			for (; r < dctblock_rowcnt; ++r) {
				for (c = 0; c < dctblock_colcnt; ++c) {
					matpos_c = c + blockCSel * dctblock_colcnt;
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r, matpos_c, dctblock[blockIndex][r][c]);
				}
				printf("\n");
			}
			++blockIndex;
		}

		//对右下角的块补齐
		if (matrix_extracol_cnt != 0) {
			for (r = 0; r < matrix_extrarow_cnt; ++r) {
				matpos_r = r + blockRSel * dctblock_rowcnt;
				for (c = 0; c < matrix_extracol_cnt; ++c) {
					matpos_c = c + blockCSel * dctblock_colcnt;
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r, matpos_c, dctblock[blockIndex][r][c]);
				}
				for (; c < dctblock_colcnt; ++c) {
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r, matpos_c, dctblock[blockIndex][r][c]);
				}
				printf("\n");
			}
			for (; r < dctblock_rowcnt; ++r) {
				for (c = 0; c < matrix_extracol_cnt; ++c) {
					matpos_c = c + blockCSel * dctblock_colcnt;
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r, matpos_c, dctblock[blockIndex][r][c]);
				}
				for (; c < dctblock_colcnt; ++c) {
					dctblock[blockIndex][r][c] = (*matrix)[matpos_r][matpos_c];
					printf("b[%d][%d][%d]=m[%d][%d]=%.1f   ", blockIndex, r, c, matpos_r, matpos_c, dctblock[blockIndex][r][c]);
				}
				printf("\n");
			}
		}
	}
	printf("\nover\n");
}


void DCTBlockSplitter::split(Matrix<float>* dctblock_y, Matrix<float>* dctblock_cb, Matrix<float>* dctblock_cr)
{
	_split(matrix_y, blockcnt_y_h, blockcnt_y_v, matrix_y_extracol_cnt, matrix_y_extrarow_cnt, dctblock_y);
	_split(matrix_cb, blockcnt_cb_h, blockcnt_cb_v, matrix_cb_extracol_cnt, matrix_cb_extrarow_cnt, dctblock_cb);
	_split(matrix_cr, blockcnt_cr_h, blockcnt_cr_v, matrix_cr_extracol_cnt, matrix_cr_extrarow_cnt, dctblock_cr);
}


