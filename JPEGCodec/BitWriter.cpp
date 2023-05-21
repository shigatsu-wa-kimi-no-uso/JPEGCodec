/*
* BitWriter.cpp
* Written by kiminouso, 2023/05/12
*/
#include "BitWriter.h"

BitWriter::BitWriter(std::vector<BYTE>& data) :_lastByteBitCnt(8), _data(data)
{

}

BitWriter::BitWriter(const BitWriter& src) :_lastByteBitCnt(src._lastByteBitCnt), _data(src._data)
{

}

void BitWriter::writeBit(const bool bit)
{
	if (_lastByteBitCnt == 8) {
		_data.push_back(0);
		_lastByteBitCnt = 0;
	}
	_data.back() |= ((BYTE)bit & 1) << (7 - _lastByteBitCnt);
	++_lastByteBitCnt;
	_stuffZeroIfNecessary();
}

void BitWriter::_stuffZeroIfNecessary()
{
	if (_data.back() == 0xFF) {
		_data.push_back(0);
	}
}

std::vector<BYTE>& BitWriter::getData()
{
	return _data;
}

void BitWriter::write(const BitString& src)
{
	int len = src.length();
	for (int i = len - 1; i >= 0; --i) {
		writeBit(src.bit(i));
	}
}

void BitWriter::fillIncompleteByteWithOne()
{
	while (_lastByteBitCnt != 8) {
		writeBit(1);
	}
}
