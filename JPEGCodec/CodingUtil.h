/*
* CodingUtil.h
* Written by kiminouso, 2023/04/06
*/
#pragma once
#ifndef CodingUtil_h__
#define CodingUtil_h__
#include "typedef.h"
#include "BitString.h"

class BitCodec {
public:
	static BitString getBitString(int val);
	static BYTE mergeToByte(int high4, int low4);
	static int getValue(const BitString& bitString);
};

class RunLengthCodec {
public:
	static void encode(const int* seq, const int seqLen, RLCode* codeBuf, int* count);
	static void decode(const RLCode* codes, const int codeCnt, const int seqLength, int* seqBuf);
};

class DPCM
{
private:
	int _preVal;
public:
	DPCM();
	void reset();
	int nextDiff(int val);
	int nextVal(int diff);
};

#endif // CodingUtil_h__
