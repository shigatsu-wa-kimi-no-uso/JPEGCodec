/*
* ColorSpaceConverter.cpp
* Written by kiminouso, 2023/04/05
*/
#include "ColorSpaceConverter.h"

float ColorSpaceConverter::_luma(const RGBTriple* rgb) {
	float y = 0.299 * rgb->rgbRed + 0.587 * rgb->rgbGreen + 0.114 * rgb->rgbBlue;
	return _normalize(y);
}

float ColorSpaceConverter::_blueChroma(const RGBTriple* rgb) {
	float cb = -0.169 * rgb->rgbRed - 0.331 * rgb->rgbGreen + 0.5 * rgb->rgbBlue + 128;
	return _normalize(cb);
}

float ColorSpaceConverter::_redChroma(const RGBTriple* rgb) {
	float cr = 0.5 * rgb->rgbRed - 0.419 * rgb->rgbGreen - 0.081 * rgb->rgbBlue + 128;
	return _normalize(cr);
}

float ColorSpaceConverter::_red(const YCbCr* ycbcr) {
	float r = ycbcr->Y + 1.400 * (ycbcr->Cr - 128);
	return _normalize(r);
}

float ColorSpaceConverter::_green(const YCbCr* ycbcr) {
	float g = ycbcr->Y - 0.343 * (ycbcr->Cb - 128) - 0.711 * (ycbcr->Cr - 128);
	return _normalize(g);
}

float ColorSpaceConverter::_blue(const YCbCr* ycbcr) {
	float b = ycbcr->Y + 1.765 * (ycbcr->Cb - 128);
	return _normalize(b);
}

float ColorSpaceConverter::_normalize(const float val)
{
	if (val > 255) {
		return 255;
	} else if (val < 0) {
		return 0;
	} else {
		return val;
	}
}

void ColorSpaceConverter::RGB2YCbCr(const RGBTriple* rgbColor, YCbCr* ycbcrColor) {
	ycbcrColor->Y = myround(_luma(rgbColor));
	ycbcrColor->Cb = myround(_blueChroma(rgbColor));
	ycbcrColor->Cr = myround(_redChroma(rgbColor));
}

void ColorSpaceConverter::RGB2YCbCr(const RGBTriple* rgbColors, const DWORD count, YCbCr* ycbcrColors) {
	for (DWORD pos = 0; pos < count; ++pos) {
		RGB2YCbCr(&rgbColors[pos], &ycbcrColors[pos]);
	}
}

void ColorSpaceConverter::RGB2YCbCr(const Matrix<RGBTriple>* rgbMatrix, Matrix<YCbCr>* ycbcrMatrix){
	for (DWORD y = 0; y < rgbMatrix->row_cnt; ++y) {
		for (DWORD x = 0; x < rgbMatrix->column_cnt; ++x) {
			RGB2YCbCr(&(*rgbMatrix)[y][x], &(*ycbcrMatrix)[y][x]);
		}
	}
}

void ColorSpaceConverter::RGB2YCbCr(const Matrix<RGBQuad>* rgbMatrix, Matrix<YCbCr>* ycbcrMatrix) {
	for (DWORD y = 0; y < rgbMatrix->row_cnt; ++y) {
		for (DWORD x = 0; x < rgbMatrix->column_cnt; ++x) {
			RGB2YCbCr(&(*rgbMatrix)[y][x], &(*ycbcrMatrix)[y][x]);
		}
	}
}

void ColorSpaceConverter::YCbCr2RGB(const YCbCr* ycbcrColor, RGBTriple* rgbColor) {
	rgbColor->rgbRed = myround(_red(ycbcrColor));
	rgbColor->rgbGreen = myround(_green(ycbcrColor));
	rgbColor->rgbBlue = myround(_blue(ycbcrColor));
}

void ColorSpaceConverter::YCbCr2RGB(const YCbCr* ycbcrColors, const DWORD count, RGBTriple* rgbColors) {
	for (DWORD pos = 0; pos < count; ++pos) {
		YCbCr2RGB(&ycbcrColors[pos], &rgbColors[pos]);
	}
}

void ColorSpaceConverter::YCbCr2RGB(const Matrix<YCbCr>* ycbcrMatrix, Matrix<RGBTriple>* rgbMatrix)
{
	for (DWORD y = 0; y < rgbMatrix->row_cnt; ++y) {
		for (DWORD x = 0; x < rgbMatrix->column_cnt; ++x) {
			YCbCr2RGB(&(*ycbcrMatrix)[y][x], &(*rgbMatrix)[y][x]);
		}
	}
}
