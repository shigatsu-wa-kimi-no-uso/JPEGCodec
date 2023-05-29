/*
* BMPFileBuilder.cpp
* Written by kiminouso, 2023/05/22
*/
#include "BMPFileBuilder.h"

void BMPFileBuilder::_writeBMPFileHeader() {
	BitmapFileHeader bmpFileHeader = {
		'MB',
		_lineSize * _height + 0x36,
		0,
		0,
		0x36
	};
	fwrite(&bmpFileHeader, sizeof(bmpFileHeader), 1, _hFile);
}

void BMPFileBuilder::_writeBMPInfoHeader() {
	BitmapInfoHeader bmpInfoHeader = {
		0x28,
		_width,
		_height,
		1,
		sizeof(RGBTriple) * 8,
		0,
		_lineSize * _height,
		0,
		0,
		0,
		0
	};
	fwrite(&bmpInfoHeader, sizeof(bmpInfoHeader), 1, _hFile);
}

void BMPFileBuilder::_writeRGBData() {
	int extraByteCnt = ALIGN(_width * sizeof(RGBTriple), 2) - _width * sizeof(RGBTriple);
	BYTE extraByte[3] = { 0 };
	for (int r = _rgbData->row_cnt - 1; r >= 0; --r) {
		fwrite(_rgbData->row_pointer(r), sizeof(RGBTriple), _width, _hFile);
		fwrite(extraByte, sizeof(BYTE), extraByteCnt, _hFile);
	}
}

BMPFileBuilder& BMPFileBuilder::setWidth(LONG width) {
	_width = width;
	_lineSize = ALIGN(_width * sizeof(RGBTriple), 2);
	return *this;
}

BMPFileBuilder& BMPFileBuilder::setHeight(LONG height) {
	_height = height;
	return *this;
}

BMPFileBuilder& BMPFileBuilder::setRGBData(const Matrix<RGBTriple>& rgbData) {
	_rgbData = &rgbData;
	return *this;
}

void BMPFileBuilder::makeBMPFile(const char* fileName) {
	_hFile = fopen(fileName, "wb");
	_writeBMPFileHeader();
	_writeBMPInfoHeader();
	_writeRGBData();
	fclose(_hFile);
}
