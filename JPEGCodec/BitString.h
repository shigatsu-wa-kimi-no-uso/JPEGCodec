/*
* BitString.h
* Written by kiminouso, 2023/05/01
*/
#pragma once
#ifndef BitString_h__
#define BitString_h__
#include <ostream>
#include "typedef.h"

class BitString {
private:
	union {
		BYTE _byte[4];
		DWORD _bits;
	};
	int _length;

	void _testOutOfBound(int index) const;
	void _testIntegrity() const;
	DWORD _mask() const;
	static int _getMSBIndex(const DWORD& val);
public:
	BitString();
	BitString(const DWORD& val);
	BitString(const DWORD& val,const int length);
	BitString(const BitString& val);
	void set1(int index);
	void set0(int index);
	void setLength(const int length);
	void push_back(bool bit);
	void pop_back();
	void push_front(bool bit);
	void pop_front();
	void reset();
	void setAll0();
	void setAll1();
	BYTE byteValue(const int& index)const;
	void append(const BitString& src);
	void moveAppend(BitString& src);
	const DWORD value() const;
	const int length() const;
	bool bit(const int& index) const;
	bool operator<(const BitString& rgt) const;
	bool operator==(const BitString& rgt) const;
	bool operator>(const BitString& rgt) const;
	BYTE operator[](const int& index) const;
	BitString operator<<(const BYTE& val);
	BitString operator>>(const BYTE& val);
	BitString operator~() const;
	friend std::ostream& operator<<(std::ostream& os, const BitString& rgt);
	BitString& operator=(const BitString& rgt);
	BitString& operator=(const DWORD& rgt);
	void operator--();
	void operator++();
	void operator+=(const BitString& rgt);
	void operator-=(const BitString& rgt);
	void operator+=(const DWORD& rgt);
	void operator-=(const DWORD& rgt);
	BitString operator+(const BitString& rgt) const;
	BitString operator+(const DWORD& rgt) const;
	BitString operator-(const BitString& rgt) const;
	BitString operator-(const DWORD& rgt) const;
};

#endif // BitString_h__
