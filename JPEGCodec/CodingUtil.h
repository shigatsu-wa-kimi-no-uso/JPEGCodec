/*
* CodingUtil.h
* Written by kiminouso, 2023/04/06
*/
#pragma once
#ifndef CodingUtil_h__
#define CodingUtil_h__
#include "typedef.h"
#include "BitString.h"

class BitEncoder {
public:
	static BitString getBitString(int val);
	static BYTE mergeToByte(int high4, int low4);
};

class RLE {
public:
	static void getRLECodes(const Block* block, RLECode* codeBuf,int* count);
};

class DPCM
{
private:
	int pred;
public:
	DPCM();
	void reset();
	int nextDiff(int val);
};

#endif // CodingUtil_h__
