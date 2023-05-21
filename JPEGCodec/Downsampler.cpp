/*
* Downsampler.cpp
* Written by kiminouso, 2023/04/05
*/
#include "Downsampler.h"
#include <stdio.h>

int Downsampler::_getSampleCount(int factor, int length)
{
	//保证 subsampFact 只取1或2, 如果采样因子是2,但长度不能被2整除,则增加一个采样点(相当于多出的那个点仍采样而不是丢弃)
	if (factor == 2) {
		if (length % 2 == 0) {
			return length>>1;
		} else {
			return (length>>1) + 1;
		}
	} else {
		return length;
	}
}

int Downsampler::getChromaRowSampleCount()
{
	return _getSampleCount(_subsampFact_V, _ycbcrColors->row_cnt);
}

int Downsampler::getChromaColumnSampleCount()
{
	return _getSampleCount(_subsampFact_H, _ycbcrColors->column_cnt);
}

void Downsampler::setChannelData(Matrix<YCbCr>* ycbcrColors)
{
	this->_ycbcrColors = ycbcrColors;
}

void Downsampler::setSubsamplingFactors(BYTE subsampFact_H, BYTE subsampFact_V)
{
	this->_subsampFact_H = subsampFact_H;
	this->_subsampFact_V = subsampFact_V;
}



void Downsampler::sample(Matrix<float>* matrix_y, Matrix<float>* matrix_cb, Matrix<float>* matrix_cr)
{
	for (DWORD i = 0; i < _ycbcrColors->row_cnt; ++i) {
		for (DWORD j = 0; j < _ycbcrColors->column_cnt; ++j) {
			(*matrix_y)[i][j] = (*_ycbcrColors)[i][j].Y;
		}
	}

	DWORD cbcr_samp_row_cnt = getChromaRowSampleCount();
	DWORD cbcr_samp_col_cnt = getChromaColumnSampleCount();

	for (DWORD i = 0; i < cbcr_samp_row_cnt; ++i) {
		for (DWORD j = 0; j < cbcr_samp_col_cnt; ++j) {
			(*matrix_cb)[i][j] = (*_ycbcrColors)[i * _subsampFact_V][j * _subsampFact_H].Cb;
			(*matrix_cr)[i][j] = (*_ycbcrColors)[i * _subsampFact_V][j * _subsampFact_H].Cr;
		}
	}
}

void Downsampler::mean_sample(Matrix<float>* matrix_y, Matrix<float>* matrix_cb, Matrix<float>* matrix_cr)
{
	for (DWORD i = 0; i < _ycbcrColors->row_cnt; ++i) {
		for (DWORD j = 0; j < _ycbcrColors->column_cnt; ++j) {
			(*matrix_y)[i][j] = (*_ycbcrColors)[i][j].Y;
		}
	}

	for (DWORD i = 0; i < _ycbcrColors->row_cnt / _subsampFact_V; ++i) {
		for (DWORD j = 0; j < _ycbcrColors->column_cnt / _subsampFact_H; ++j) {
			float sum_cb = 0, sum_cr = 0;
			for (DWORD p = 0; p < _subsampFact_V; ++p) {
				for (DWORD q = 0; q < _subsampFact_H; ++q) {
					sum_cb += (*_ycbcrColors)[i * _subsampFact_V + p][j * _subsampFact_H + q].Cb;
					sum_cr += (*_ycbcrColors)[i * _subsampFact_V + p][j * _subsampFact_H + q].Cr;
				}
			}
			(*matrix_cb)[i][j] = sum_cb / (float)(_subsampFact_V * _subsampFact_H);
			(*matrix_cr)[i][j] = sum_cr / (float)(_subsampFact_V * _subsampFact_H);
		}
	}

	DWORD lastSamp_row_index = getChromaRowSampleCount() - 1;
	DWORD lastSamp_col_index = getChromaColumnSampleCount() - 1;
	int lr_corner_neednt_complement = -2;		//右下角是否需要补充标记, 为0说明需要补充
	//检测行是否需要补充采样
	if (lastSamp_row_index == _ycbcrColors->row_cnt / _subsampFact_V) {
		for (DWORD j = 0; j < _ycbcrColors->column_cnt / _subsampFact_H; ++j) {
			float sum_cb = 0, sum_cr = 0;
			for (DWORD q = 0; q < _subsampFact_H; ++q) {
				sum_cb += (*_ycbcrColors)[lastSamp_row_index * _subsampFact_V][j * _subsampFact_H + q].Cb;
				sum_cr += (*_ycbcrColors)[lastSamp_row_index * _subsampFact_V][j * _subsampFact_H + q].Cr;
			}
			(*matrix_cb)[lastSamp_row_index][j] = sum_cb / (float)(_subsampFact_H);
			(*matrix_cr)[lastSamp_row_index][j] = sum_cr / (float)(_subsampFact_H);
		}
		++lr_corner_neednt_complement;
	}

	//检测列是否需要补充采样
	if (lastSamp_col_index == _ycbcrColors->column_cnt / _subsampFact_H) {
		for (DWORD i = 0; i < _ycbcrColors->row_cnt / _subsampFact_V; ++i) {
			float sum_cb = 0, sum_cr = 0;
			for (DWORD p = 0; p < _subsampFact_V; ++p) {
				sum_cb += (*_ycbcrColors)[i * _subsampFact_V + p][lastSamp_col_index * _subsampFact_H].Cb;
				sum_cr += (*_ycbcrColors)[i * _subsampFact_V + p][lastSamp_col_index * _subsampFact_H].Cr;
			}
			(*matrix_cb)[i][lastSamp_col_index] = sum_cb / (float)(_subsampFact_V);
			(*matrix_cr)[i][lastSamp_col_index] = sum_cr / (float)(_subsampFact_V);
		}
		++lr_corner_neednt_complement;
	}


	if (lr_corner_neednt_complement == 0) {
		(*matrix_cb)[lastSamp_row_index][lastSamp_col_index]= (*_ycbcrColors)[lastSamp_row_index * _subsampFact_V ][lastSamp_col_index * _subsampFact_H].Cb;
		(*matrix_cr)[lastSamp_row_index][lastSamp_col_index] = (*_ycbcrColors)[lastSamp_row_index * _subsampFact_V][lastSamp_col_index * _subsampFact_H].Cr;
	}
	
}
