/*
* BMPFileBuilder.h
* Written by kiminouso, 2023/05/22
*/
#pragma once
#ifndef BMPFileBuilder_h__
#define BMPFileBuilder_h__
#include "typedef.h"

class BMPFileBuilder {
private:
	FILE* _hFile;
	LONG _width;
	LONG _height;
	DWORD _lineSize;
	const Matrix<RGBTriple>* _rgbData;
	void _writeBMPFileHeader() {
		BitmapFileHeader bmpFileHeader = {
			'MB',
			_lineSize * _height + 0x36,
			0,
			0,
			0x36
		};
		fwrite(&bmpFileHeader, sizeof(bmpFileHeader), 1, _hFile);
	}

	void _writeBMPInfoHeader() {
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

	void _writeRGBData() {
		int extraByteCnt = _lineSize * _height - _width * 3;
		BYTE extraByte[3] = { 0 };
		for (int r = _rgbData->row_cnt - 1; r >= 0; --r) {
			fwrite(_rgbData->row_pointer(r), sizeof(RGBTriple), _width, _hFile);
			fwrite(extraByte, sizeof(BYTE), extraByteCnt, _hFile);
		}
	}
public:
	BMPFileBuilder& setWidth(LONG width) {
		_width = width;
		_lineSize = ALIGN(_width * sizeof(RGBTriple), 2);
	}
	BMPFileBuilder& setHeight(LONG height) {
		_height = height;
	}
	BMPFileBuilder& setRGBData(const Matrix<RGBTriple>& rgbData) {
		_rgbData = &rgbData;
	}
	void makeBMPFile(const char* fileName) {
		_hFile = fopen(fileName, "wb");
		_writeBMPFileHeader();
		_writeBMPInfoHeader();
		_writeRGBData();
		fclose(_hFile);
	}
};

#endif // BMPFileBuilder_h__
