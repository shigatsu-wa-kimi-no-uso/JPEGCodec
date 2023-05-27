/*
* ColorSpaceConverter.h
* Written by kiminouso, 2023/04/05
*/
#pragma once
#ifndef ColorSpaceConverter_h__
#define ColorSpaceConverter_h__
#include "typedef.h"

class ColorSpaceConverter
{
private:
	float _luma(const RGBTriple& rgb);
	float _blueChroma(const RGBTriple& rgb);
	float _redChroma(const RGBTriple& rgb);
	float _red(const YCbCr& ycbcr);
	float _green(const YCbCr& ycbcr);
	float _blue(const YCbCr& ycbcr);
	float _normalize(const float val);
public:
	void RGB2YCbCr(const RGBTriple& rgbColor, YCbCr& ycbcrColor);
	void RGB2YCbCr(const Matrix<RGBTriple>& rgbMatrix, Matrix<YCbCr>& ycbcrMatrix);
	void RGB2YCbCr(const Matrix<RGBQuad>& rgbMatrix, Matrix<YCbCr>& ycbcrMatrix);
	void YCbCr2RGB(const YCbCr& ycbcrColor, RGBTriple& rgbColor);
	void YCbCr2RGB(const Matrix<YCbCr>& ycbcrMatrix, Matrix<RGBTriple>& rgbMatrix);
};

#endif // ColorSpaceConverter_h__