/*
* BMPFile.h
* Written by kiminouso, 2023/04/05
*/
#pragma once
#ifndef BMPFile_h__
#define BMPFile_h__
#include "typedef.h"
#include <stdio.h>

class BMPFile
{
private:
	FILE* hFile;
	BitmapFileHeader _fileHeader;
	BitmapInfoHeader _infoHeader;
	DWORD _rowSize;
	size_t _fileSize;
	DWORD _extraByteCnt;
	DWORD _getRowSize(LONG width, WORD bitCount);
	bool _testLegality();
	bool _readRGB(Matrix<RGBTriple>* rgbMatrix, LONG ypos_start, LONG ypos_end);
public:
	WORD getBitCount();
	bool load(const char* filepath);
	LONG width();
	LONG height();
	bool readRGB(Matrix<RGBTriple>* rgbMatrix);
	bool readRGB(Matrix<RGBQuad>* rgbMatrix);

	[[deprecated]]
	bool readRGB(Matrix<RGBQuad>* rgbMatrix, LONG y_start, LONG y_end);

	[[deprecated]]
	bool readRGB(Matrix<RGBTriple>* rgbMatrix,LONG y_start,LONG y_end);
};

#endif // BMPFile_h__