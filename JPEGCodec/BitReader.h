/*
* BitReader.h
* Written by kiminouso, 2023/05/22
*/

#pragma once
#ifndef BitReader_h__
#define BitReader_h__
#include <vector>
#include "typedef.h"
#include "BitString.h"

class BitReader
{
private:
	int _bitPos;
	int _bytePos;
	const std::vector<BYTE>& _data;

	void _moveOn(){
		++_bitPos;
		if (_bitPos == 8) {
			++_bytePos;
			_bitPos = 0;
		}
	}

public:
	BitReader(const std::vector<BYTE>& data) :_data(data),_bitPos(0),_bytePos(0) {}
	BitReader(const BitReader& val) :_data(val._data), _bitPos(val._bitPos), _bytePos(val._bytePos) {}
	//每个字节均从MSB开始读
	BYTE readBit() {
		BYTE bit = 0x1&(_data[_bytePos] >> (7 - _bitPos));
		_moveOn();
		return bit;
	}
	BitString readBits(int length) {
		assert(length < 33 && length >= 0);
		BitString bits;
		while (length!=0) {
			bits.push_back(readBit());
			--length;
		}
		return bits;
	}
	bool hasNext() {
		return !(_data.size() == _bytePos);
	}
	DWORD bytePosition() const {
		return _bytePos;
	}
	DWORD bitPosition() const {
		return _bitPos;
	}
}
#endif // BitReader_h__