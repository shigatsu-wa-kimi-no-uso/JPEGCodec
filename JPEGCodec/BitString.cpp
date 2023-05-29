/*
* BitString.cpp
* Written by kiminouso, 2023/05/01
*/
#include<assert.h>
#include<ostream>
#include "BitString.h"

void BitString::_testOutOfBound(int index) const {
	assert(index >= 0 && index < _length);
}

void BitString::_testIntegrity() const {
	assert(_length <= 8 * sizeof(_bits) && _length >= 0);
}

DWORD BitString::_mask() const {
	return (1 << _length) - 1;
}

int BitString::_getMSBIndex(const DWORD& val) {
	int ub = 32;
	int lb = 0;
	while (lb <= ub) {
		int mid = lb + (ub - lb) / 2;
		DWORD mask = ~0U << mid;
		DWORD maskedVal = val & mask;
		if (maskedVal == 0) {
			ub = mid - 1;
		} else if (maskedVal == ~mask + 1) {
			return mid + 1;
		} else {
			lb = mid + 1;
		}
	}
	return 0;
}

BitString::BitString() :_bits(0), _length(0) {}

BitString::BitString(const DWORD& val) : _bits(val), _length(_getMSBIndex(val)) {}

BitString::BitString(const DWORD& val, const int length) : _bits(val), _length(length) {}

BitString::BitString(const BitString& val) : _bits(val._bits), _length(val._length) {}

void BitString::set1(int index) {
	_testOutOfBound(index);
	_bits |= (1 << index);
}

void BitString::set0(int index) {
	_testOutOfBound(index);
	_bits &= (~(1 << index));
}

void BitString::setLength(const int length) {
	_length = length;
}

void BitString::push_back(bool bit) {
	_bits <<= 1;
	_bits |= (DWORD)bit;
	_length++;
	_testIntegrity();
}

void BitString::pop_back() {
	_bits >>= 1;
	_length--;
	_testIntegrity();
}

void BitString::push_front(bool bit) {
	_bits |= (bit << _length);
	_length++;
	_testIntegrity();
}

void BitString::pop_front() {
	_length--;
	_testIntegrity();
}

void BitString::reset() {
	_bits = 0;
	_length = 0;
}

void BitString::setAll0() {
	_bits = 0;
}

void BitString::setAll1() {
	_bits = ~0;
}

BYTE BitString::byteValue(const int& index) const {
	int boundByte = _length / 8;
	BYTE mask = (1 << (_length - boundByte * 8)) - 1;
	if (boundByte == index) {
		return _byte[index] & mask;
	} else if (index < boundByte) {
		return _byte[index];
	} else {
		return 0;
	}
}

void BitString::append(const BitString& src) {
	int shiftBitCnt = (src._length >(8 - _length)) ? 8 - _length : src._length;
	_bits <<= shiftBitCnt;
	DWORD mask = (1 << shiftBitCnt) - 1;
	_bits |= mask & (src._bits >> (src._length - shiftBitCnt));
	_length += shiftBitCnt;
}

void BitString::moveAppend(BitString& src) {
	int shiftBitCnt = (src._length > (8 * sizeof(_bits) - _length)) ? 8 * sizeof(_bits) - _length : src._length;
	_bits <<= shiftBitCnt;
	DWORD mask = (1 << shiftBitCnt) - 1;
	mask <<= (src._length - shiftBitCnt);
	_bits |= mask & (src._bits >> (src._length - shiftBitCnt));
	src._length -= shiftBitCnt;
	_length += shiftBitCnt;
}

const DWORD BitString::value() const {
	return _bits & _mask();
}

const int BitString::length() const {
	return _length;
}

bool BitString::bit(const int& index) const {
	return (_bits >> index) & 1;
}

bool BitString::operator<(const BitString& rgt) const {
	return value() < rgt.value();
}

bool BitString::operator==(const BitString& rgt) const {
	return value() == rgt.value();
}

bool BitString::operator>(const BitString& rgt) const {
	return !(*this < rgt || *this == rgt);
}

BYTE BitString::operator[](const int& index) const {
	return byteValue(index);
}

BitString BitString::operator<<(const BYTE& val) {
	BitString bitString(*this);
	bitString._bits <<= val;
	return bitString;
}

BitString BitString::operator>>(const BYTE& val) {
	BitString bitString(*this);
	bitString._bits >>= val;
	return bitString;
	set0(_length - 1);
}

BitString BitString::operator~() const {
	BitString bitString(*this);
	bitString._bits = ~bitString._bits;
	return bitString;
}

std::ostream& operator<<(std::ostream& os, const BitString& rgt){
	static char str[32];
	int index = 0;
	for (int i = rgt._length - 1; i >= 0; --i) {
		str[index++] = rgt.bit(i) + '0';
	}
	str[index] = 0;
	os << str;
	return os;
}

BitString& BitString::operator=(const BitString& rgt) {
	_bits = rgt._bits;
	_length = rgt._length;
	return *this;
}

BitString& BitString::operator=(const DWORD& rgt) {
	_bits = rgt;
	_length = _getMSBIndex(rgt);
	return *this;
}

void BitString::operator--() {
	_bits--;
}
void BitString::operator++() {
	_bits++;
}

void BitString::operator+=(const BitString& rgt) {
	_bits += rgt.value();
}

void BitString::operator-=(const BitString& rgt) {
	_bits -= rgt.value();
}

void BitString::operator+=(const DWORD& rgt) {
	_bits += rgt;
}

void BitString::operator-=(const DWORD& rgt) {
	_bits -= rgt;
}

BitString BitString::operator+(const BitString& rgt) const {
	BitString bitString(*this);
	bitString += rgt;
	return bitString;
}

BitString BitString::operator+(const DWORD& rgt) const {
	BitString bitString(*this);
	bitString += rgt;
	return bitString;
}

BitString BitString::operator-(const BitString& rgt) const {
	BitString bitString(*this);
	bitString -= rgt;
	return bitString;
}

BitString BitString::operator-(const DWORD& rgt)  const {
	BitString bitString(*this);
	bitString -= rgt;
	return bitString;
}
