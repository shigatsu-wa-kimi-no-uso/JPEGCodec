/*
* Downsampler.h
* Written by kiminouso, 2023/04/05
*/
#pragma once
#include "typedef.h"

class Downsampler
{
private:
	Matrix<YCbCr>* _ycbcrColors;
	BYTE _subsampFact_H;
	BYTE _subsampFact_V;
	
public:
	static int _getSampleCount(int factor, int length);
	int getChromaRowSampleCount();
	int getChromaColumnSampleCount();
	void setChannelData(Matrix<YCbCr>* ycbcrColors);
	void setSubsamplingFactors(BYTE subsampFact_H, BYTE subsampFact_V);
	void sample(Matrix<float>* matrix_y, Matrix<float>* matrix_cb, Matrix<float>* matrix_cr);
	void mean_sample(Matrix<float>* matrix_y, Matrix<float>* matrix_cb, Matrix<float>* matrix_cr);
};

