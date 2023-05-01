#pragma once
#ifndef DCTBlockSplitter_h__
#define DCTBlockSplitter_h__
#include "typedef.h"

class DCTBlockSplitter
{
private:
	static constexpr int dctblock_colcnt = BLOCK_COLCNT;
	static constexpr int dctblock_rowcnt = BLOCK_ROWCNT;
	Matrix<float>* matrix_y;
	Matrix<float>* matrix_cb;
	Matrix<float>* matrix_cr;
	DWORD matrix_y_extracol_cnt;		//需要补充的列数
	DWORD matrix_y_extrarow_cnt;		//需要补充的行数
	DWORD matrix_cb_extracol_cnt;		//需要补充的列数
	DWORD matrix_cb_extrarow_cnt;		//需要补充的行数
	DWORD matrix_cr_extracol_cnt;		//需要补充的列数
	DWORD matrix_cr_extrarow_cnt;		//需要补充的行数
	DWORD blockcnt_y_h;					//分量矩阵的水平方向上可以分为多少DCT块(不包含不能被完整地分为8X8块的部分)
	DWORD blockcnt_y_v;					//分量矩阵的竖直方向上可以分为多少DCT块
	DWORD blockcnt_cb_h;	
	DWORD blockcnt_cb_v;
	DWORD blockcnt_cr_h;
	DWORD blockcnt_cr_v;

	void _split(const Matrix<float>* matrix,const DWORD blockcnt_h,const DWORD blockcnt_v,const DWORD matrix_extracol_cnt,const DWORD matrix_extrarow_cnt, Matrix<float>* dctblock);
public:
	void setYCmptMatrix(Matrix<float>* matrix_y);
	void setCbCmptMatrix(Matrix<float>* matrix_cb);
	void setCrCmptMatrix(Matrix<float>* matrix_cr);
	DWORD getYBlockCount();
	DWORD getCbBlockCount();
	DWORD getCrBlockCount();
	void split(Matrix<float>* dctblock_y, Matrix<float>* dctblock_cb, Matrix<float>* dctblock_cr);	//8X8分块,输出到传入的参数指向的buffer
};

#endif // DCTBlockSplitter_h__