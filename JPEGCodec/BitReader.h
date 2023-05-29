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
	int _bitPos;	//��һ��Ҫ����bitλ��
	int _bytePos;	//��һ��Ҫ����bit���ڵ�byteλ��
	const std::vector<BYTE>& _data;

	void _moveOn();
public:
	BitReader(const std::vector<BYTE>& data);
	BitReader(const BitReader& val);
	//ÿ���ֽھ���MSB��ʼ��
	BYTE readBit();
	BitString readBits(int length);
	bool hasNext();
	DWORD bytePosition() const;
	DWORD bitPosition() const;
};

#endif // BitReader_h__