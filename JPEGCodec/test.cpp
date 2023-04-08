/*
* test.cpp
* Written by kiminouso, 2023/04/05
*/

#include "BMPFile.h"
#include "ColorSpaceConverter.h"

/*
int main() {
	BMPFile bmpFile;
	bmpFile.load("C:\\Users\\Administrator\\Pictures\\img.bmp");
	Matrix<RGBTriple> rgbMat = Matrix<RGBTriple>(bmpFile.height(), bmpFile.width());
	bool res = bmpFile.readRGB(&rgbMat);
	if (!res) {
		perror("Read data failed.");
	}
	bmpFile.close();
	ColorSpaceConverter csc;
	Matrix<YCbCr> ycbcrMat = Matrix<YCbCr>(bmpFile.height(), bmpFile.width());
	csc.RGB2YCbCr(&rgbMat, &ycbcrMat);
	FILE* out = fopen("output.yuv", "wb");

	for (int i = 0; i < ycbcrMat.row_cnt; ++i) {
		for (int j = 0; j < ycbcrMat.column_cnt; ++j) {
			BYTE val = round(ycbcrMat[i][j].Y);
			fwrite(&val, 1, 1, out);
		}
	}

	for (int i = 0; i < ycbcrMat.row_cnt; ++i) {
		for (int j = 0; j < ycbcrMat.column_cnt; ++j) {
			BYTE val = round(ycbcrMat[i][j].Cb);
			fwrite(&val, 1, 1, out);
		}
	}

	for (int i = 0; i < ycbcrMat.row_cnt; ++i) {
		for (int j = 0; j < ycbcrMat.column_cnt; ++j) {
			BYTE val = round(ycbcrMat[i][j].Cr);
			fwrite(&val, 1, 1, out);
		}
	}
	return 0;
}*/