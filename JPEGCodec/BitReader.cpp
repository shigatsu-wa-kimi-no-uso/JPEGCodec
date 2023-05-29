/*
* BitReader.h
* Written by kiminouso, 2023/05/22
*/
#include "BitReader.h"

void BitReader::_moveOn() {
	++_bitPos;
	if (_bitPos == 8) {
		++_bytePos;
		_bitPos = 0;
	}
}

BitReader::BitReader(const std::vector<BYTE>& data) :_data(data), _bitPos(0), _bytePos(0) {
}

BitReader::BitReader(const BitReader& val) : _data(val._data), _bitPos(val._bitPos), _bytePos(val._bytePos) {
}

//每个字节均从MSB开始读
BYTE BitReader::readBit() {
	BYTE bit = 0x1 & (_data[_bytePos] >> (7 - _bitPos));
	_moveOn();
	return bit;
}

BitString BitReader::readBits(int length) {
	assert(length < 33 && length >= 0);
	BitString bits;
	while (length != 0) {
		bits.push_back(readBit());
		--length;
	}
	return bits;
}

bool BitReader::hasNext() {
	return !(_data.size() == _bytePos);
}

DWORD BitReader::bytePosition() const {
	return _bytePos;
}

DWORD BitReader::bitPosition() const {
	return _bitPos;
}
