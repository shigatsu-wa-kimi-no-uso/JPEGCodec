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
	int _bitPos;	//下一个要读的bit位置
	int _bytePos;	//下一个要读的bit所在的byte位置
	const std::vector<BYTE>& _data;

	void _moveOn();
public:
	BitReader(const std::vector<BYTE>& data);
	BitReader(const BitReader& val);
	//每个字节均从MSB开始读
	BYTE readBit();
	BitString readBits(int length);
	bool hasNext();
	DWORD bytePosition() const;
	DWORD bitPosition() const;
};

#endif // BitReader_h__