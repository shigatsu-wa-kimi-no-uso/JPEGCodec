/*
* BMPFile.cpp
* Written by kiminouso, 2023/04/05
*/
#include "BMPFile.h"

DWORD BMPFile::_getRowSize(LONG width, WORD bitCount) {
	return ((((width * bitCount) + 31) & ~31) >> 3);
}

bool BMPFile::_testLegality() {
	bool isOK = _fileHeader.bfType == 'MB'
		&& _fileHeader.bfSize == _fileSize
		&& (_infoHeader.biBitCount == 24 || _infoHeader.biBitCount == 32)
		&& (_infoHeader.biCompression == 0)
		&& (_infoHeader.biSizeImage == _rowSize * _infoHeader.biHeight);
	return isOK;
}

[[deprecated]]
bool BMPFile::_readRGB(Matrix<RGBTriple>* rgbMatrix, LONG ypos_start, LONG ypos_end)
{
	if (fseek(hFile, _fileHeader.bfOffBits + ypos_start*_rowSize, SEEK_SET) != 0) {
		return false;
	}
	bool isOK = false;
	for (LONG ypos = ypos_start; ypos < ypos_end; ++ypos) {
		for (LONG xpos = 0; xpos < _infoHeader.biWidth; ++xpos) {
			isOK = true
				&& 1 == fread(&(*rgbMatrix)[_infoHeader.biHeight - 1 - ypos][xpos], sizeof(RGBTriple), 1, hFile)
				&& 0 == fseek(hFile, (_infoHeader.biBitCount / 8) - sizeof(RGBTriple), SEEK_CUR); //忽略32位深度情况下多余的1字节
			if (!isOK) {
				return false;
			}
		}
		fseek(hFile, _extraByteCnt, SEEK_CUR); //跳过为补齐4字节而多出的字节
	}
	return isOK;
}

WORD BMPFile::getBitCount() {
	return _infoHeader.biBitCount;
}

bool BMPFile::load(const char* filepath) {
	hFile = fopen(filepath, "rb");
	if (hFile == NULL) {
		return false;
	}
	fseek(hFile, 0, SEEK_END);
	_fileSize = ftell(hFile);
	fseek(hFile, 0, SEEK_SET);
	size_t size;
	bool isOK = (size = fread(&_fileHeader, sizeof(BitmapFileHeader), 1, hFile)) == 1
		&& (size = fread(&_infoHeader, sizeof(BitmapInfoHeader), 1, hFile)) == 1;
	if (isOK) {
		_rowSize = _getRowSize(_infoHeader.biWidth, _infoHeader.biBitCount);
		_extraByteCnt = _rowSize - _infoHeader.biWidth * (_infoHeader.biBitCount / 8);
		isOK = _testLegality();
	}
	return isOK;
}

LONG BMPFile::width() {
	return _infoHeader.biWidth;
}

LONG BMPFile::height() {
	return _infoHeader.biHeight;
}

bool BMPFile::readRGB(Matrix<RGBTriple>* rgbMatrix) {
	if (fseek(hFile, _fileHeader.bfOffBits, SEEK_SET) != 0) {
		return false;
	}
	bool isOK = false;
	for (LONG ypos = 0; ypos < _infoHeader.biHeight; ++ypos) {
		int matrix_y_pos = _infoHeader.biHeight - 1 - ypos;
		isOK = true
			&& rgbMatrix->column_cnt == fread(rgbMatrix->row_pointer(matrix_y_pos), sizeof(RGBTriple), rgbMatrix->column_cnt, hFile)
			&& 0 == fseek(hFile, _extraByteCnt, SEEK_CUR); //跳过为补齐4字节而多出的字节
		if (!isOK) {
			return false;
		}
	}
	return isOK;
}

bool BMPFile::readRGB(Matrix<RGBQuad>* rgbMatrix)
{
	if (fseek(hFile, _fileHeader.bfOffBits , SEEK_SET) != 0) {
		return false;
	}
	bool isOK = false;

	for (LONG ypos = 0; ypos < _infoHeader.biHeight; ++ypos) {
		int matrix_y_pos = _infoHeader.biHeight - 1 - ypos;
		isOK = rgbMatrix->column_cnt == fread(rgbMatrix->row_pointer(matrix_y_pos), sizeof(RGBQuad), rgbMatrix->column_cnt, hFile);
		if (!isOK) {
			return false;
		}
	}
	return isOK;
}

[[deprecated]]
bool BMPFile::readRGB(Matrix<RGBQuad>* rgbMatrix, LONG y_start, LONG y_end)
{
	if (fseek(hFile, _fileHeader.bfOffBits + y_start * _rowSize, SEEK_SET) != 0) {
		return false;
	}
	bool isOK = false;

	for (LONG ypos = y_start; ypos < y_end; ++ypos) {
		int matrix_y_pos = rgbMatrix->row_cnt - 1 - (ypos - y_start);
		isOK = rgbMatrix->column_cnt == fread(rgbMatrix->row_pointer(matrix_y_pos), sizeof(RGBQuad), rgbMatrix->column_cnt, hFile);
		if (!isOK) {
			return false;
		}
	}
	return isOK;
}

[[deprecated]]
bool BMPFile::readRGB(Matrix<RGBTriple>* rgbMatrix, LONG y_start, LONG y_end)
{
	if (fseek(hFile, _fileHeader.bfOffBits + y_start * _rowSize, SEEK_SET) != 0) {
		return false;
	}
	bool isOK = false;
	for (LONG ypos = y_start; ypos < y_end; ++ypos) {
		int matrix_y_pos = rgbMatrix->row_cnt - 1 - (ypos - y_start);
		isOK = true 
			&& rgbMatrix->column_cnt == fread(rgbMatrix->row_pointer(matrix_y_pos), sizeof(RGBTriple), rgbMatrix->column_cnt, hFile)
			&& 0 == fseek(hFile, _extraByteCnt, SEEK_CUR); //跳过为补齐4字节而多出的字节
		if (!isOK) {
			return false;
		}
	}
	return isOK;
}
