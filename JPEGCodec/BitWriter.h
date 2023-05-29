/*
* BitWriter.h
* Written by kiminouso, 2023/05/12
*/
#pragma once
#ifndef BitWriter_h__
#define BitWriter_h__
#include <vector>
#include "typedef.h"
#include "BitString.h"

class BitWriter
{
private:
	int _lastByteBitCnt;
	std::vector<BYTE>& _data;

	const BYTE _mask() const;
public:
	BitWriter(std::vector<BYTE>& data);
	BitWriter(const BitWriter& src);
	//先写高位再写低位
	void writeBit(const bool bit);
	void _stuffZeroIfNecessary();
	std::vector<BYTE>& getData();
	void write(const BitString& src);
	void fillIncompleteByteWithOne();
};

#endif // BitWriter_h__

