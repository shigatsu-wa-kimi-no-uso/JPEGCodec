/*
* CodingUtil.h
* Written by kiminouso, 2023/04/06
*/
#pragma once
#ifndef CodingUtil_h__
#define CodingUtil_h__
#include "BitString.h"

class BitEncoder {
public:
	static BitString getBitString(int val) {
		if (val < 0) {
			val = -val;
			BitString bitString(val);
			bitString = ~bitString;
			return bitString;
		}
		return BitString(val);
	}
	static BYTE mergeToByte(int high4, int low4) {
		return ((0xF0&(BYTE)(high4 << 4)) | (0x0F & (BYTE)low4));
	}
};



class RLE {
private:
	static DWORD _zigzag[8][8];
public:
	static void getRLECodes(Block* block, RLECode* codeBuf,int* count) {
		int zeroCnt = 0;
		int zigzagged[BLOCK_COLCNT * BLOCK_ROWCNT];
		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				zigzagged[_zigzag[r][c]] = ((*block))[r][c];		
			}
		}
		for (int i = 1; i < 64; ++i) {
			printf("%2d ", zigzagged[i]);
		}
		int codePos = 0;
		//ÕÒµ½EOBµÄÎ»ÖÃ
		int EOBPos;
		for (EOBPos = BLOCK_COLCNT * BLOCK_ROWCNT - 1; EOBPos >= 0; --EOBPos) {
			if (zigzagged[EOBPos] != 0) {
				EOBPos++;
				break;
			}
		}
		//RLE
		for (int i = 1; i < EOBPos; ++i) {
			int curr = zigzagged[i];
			if (zeroCnt != 15 && curr == 0) {
				zeroCnt++;
			} else {
				codeBuf[codePos].zeroCnt = zeroCnt;
				codeBuf[codePos].value = curr;
				codePos++;
				zeroCnt = 0;
			}
		}
		if (EOBPos != BLOCK_COLCNT * BLOCK_ROWCNT - 1) {
			codeBuf[codePos].zeroCnt = 0;
			codeBuf[codePos].value = 0;
		}
		*count = codePos;
	}
};

class DPCM
{
private:
	static int pred;
public:
	static void reset() {
		pred = 0;
	}
	static int nextDiff(int val) {
		int res = val - pred;
		pred = val;
		return res;
	}
};

#endif // CodingUtil_h__
