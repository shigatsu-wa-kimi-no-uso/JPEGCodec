#pragma once
#include"typedef.h"
#include<assert.h>
#include<ostream>

class BitString {
private:
	BYTE _bits;
	int _bitCount;
	int _length;
#ifdef DEBUG
	void _testOutOfBound(int index) const {
		assert(index >= 0 && index < _length);
	}
	void _testIntegrity() const {
		assert(_length <= 8*sizeof(BYTE) && _length >= 0);
	}
#else
	void _testBound(int index) const{}
	void _testIntegrity() const{}
#endif
	BYTE _mask() const {
		return (1 << _length) - 1;
	}
public:
	static int _getMSBIndex(const BYTE& val) {
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
	BitString():_bits(0),_length(0){}
	BitString(const BYTE& val) :_bits(val), _length(_getMSBIndex(val)) {}
	BitString(const BitString& val) :_bits(val._bits), _length(val._length) {}
	void set1(int index) {
		_testOutOfBound(index);
		_bits |= (1 << index);
	}
	void set0(int index) {
		_testOutOfBound(index);
		_bits &= (~(1 << index));
	}
	void push_back(bool bit) {
		_bits <<= 1;
		_bits |= bit;
		_length++;
		_testIntegrity();
	}
	void pop_back() {
		_bits >>= 1;
		_length--;
		_testIntegrity();
	}
	void push_front(bool bit) {
		_bits |= (bit << _length);
		_length++;
		_testIntegrity();
	}
	void pop_front() {
		_length--;
		_testIntegrity();
	}

	void reset() {
		_bits = 0;
		_length = 0;
	}

	void setAll0() {
		_bits = 0;
	}
	void setAll1() {
		_bits = ~0;
	}

	void append(const BitString& src) {
		int shiftBitCnt = (src._length > (8 - _length)) ? 8 - _length : src._length;
		_bits <<= shiftBitCnt;
		BYTE mask = (1 << shiftBitCnt) - 1;
		_bits |= mask & (src._bits >> (src._length - shiftBitCnt));
		_length += shiftBitCnt;
	}

	void moveAppend(BitString& src){
		int shiftBitCnt = (src._length > (8 - _length)) ? 8 - _length : src._length;
		_bits <<= shiftBitCnt;
		BYTE mask = (1<<shiftBitCnt) - 1;
		mask <<= (src._length - shiftBitCnt);
		_bits |= mask & (src._bits >> (src._length - shiftBitCnt));
		src._length -= shiftBitCnt;
		_length += shiftBitCnt;
	}

	const BYTE value() const{
		return _bits & _mask();
	}
	const int length() const {
		return _length;
	}


	bool operator<(const BitString& rgt) const {
		return value() < rgt.value();
	}

	bool operator==(const BitString& rgt) const {
		return value() == rgt.value();
	}

	bool operator>(const BitString& rgt) const {
		return !(*this < rgt || *this == rgt);
	}

	bool operator[](const int& index) const {
		_testOutOfBound(index);
		return (_bits >> index) & 1;
	}

	BitString operator<<(const BYTE& val) {
		_bits <<= val;
	}
	BitString operator>>(const BYTE& val) {
		_bits >>= val;
		set0(_length - 1);
	}
	BitString operator~() {
		_bits = ~_bits;
	}
	friend std::ostream& operator<<(std::ostream& os, BitString& rgt)
	{
		static char str[32];
		int index = 0;
		for (int i = rgt._length - 1; i >= 0; --i) {
			str[index++] = rgt[i]+'0';
		}
		str[index] = 0;
		os << str;
		return os;
	}
	BitString& operator=(const BitString& rgt) {
		_bits = rgt._bits;
		_length = rgt._length;
		return *this;
	}
	BitString& operator=(const BYTE& rgt) {
		_bits = rgt;
		_length = _getMSBIndex(rgt);
		return *this;
	}
	
	void operator--() {
		_bits--;
	}
	void operator++() {
		_bits++;
	}

	void operator+=(const BitString& rgt) {
		_bits += rgt.value();
	}
	
	void operator-=(const BitString& rgt) {
		_bits -= rgt.value();
	}

	void operator+=(const BYTE& rgt) {
		_bits += rgt;
	}

	void operator-=(const BYTE& rgt) {
		_bits -= rgt;
	}

	BitString operator+(const BitString& rgt) {
		BitString bitString(*this);
		bitString += rgt;
		return bitString;
	}
	BitString operator+(const BYTE& rgt) {
		BitString bitString(*this);
		bitString += rgt;
		return bitString;
	}
	BitString operator-(const BitString& rgt) {
		BitString bitString(*this);
		bitString -= rgt;
		return bitString;
	}
	BitString operator-(const BYTE& rgt) {
		BitString bitString(*this);
		bitString -= rgt;
		return bitString;
	}

};