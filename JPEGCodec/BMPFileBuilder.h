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

	void _writeBMPFileHeader();
	void _writeBMPInfoHeader();
	void _writeRGBData();
public:
	BMPFileBuilder& setWidth(LONG width);
	BMPFileBuilder& setHeight(LONG height);
	BMPFileBuilder& setRGBData(const Matrix<RGBTriple>& rgbData);
	void makeBMPFile(const char* fileName);
};

#endif // BMPFileBuilder_h__
