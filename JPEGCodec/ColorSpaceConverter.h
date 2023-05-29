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
	static float _luma(const RGBTriple& rgb);
	static float _blueChroma(const RGBTriple& rgb);
	static float _redChroma(const RGBTriple& rgb);
	static float _red(const YCbCr& ycbcr);
	static float _green(const YCbCr& ycbcr);
	static float _blue(const YCbCr& ycbcr);
	static float _normalize(const float val);
public:
	static void RGB2YCbCr(const RGBTriple& rgbColor, YCbCr& ycbcrColor);
	static void RGB2YCbCr(const Matrix<RGBTriple>& rgbMatrix, Matrix<YCbCr>& ycbcrMatrix);
	static void RGB2YCbCr(const Matrix<RGBQuad>& rgbMatrix, Matrix<YCbCr>& ycbcrMatrix);
	static void YCbCr2RGB(const YCbCr& ycbcrColor, RGBTriple& rgbColor);
	static void YCbCr2RGB(const Matrix<YCbCr>& ycbcrMatrix, Matrix<RGBTriple>& rgbMatrix);
};

#endif // ColorSpaceConverter_h__