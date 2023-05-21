/*
* CodingUtil.cpp
* Written by kiminouso, 2023/04/06
*/
#include "CodingUtil.h"
#include "UtilFunc.h"


void RLE::getRLECodes(const Block* block, RLECode* codeBuf, int* count){
	int zeroCnt = 0;
	int zigzagged[BLOCK_COLCNT * BLOCK_ROWCNT];
	getZigzaggedSequence((float*)*block, zigzagged);
#ifdef TRACE
	printf("zigzagged:\n");
	for (int i = 0; i < 64; ++i) {
		printf("%d ", zigzagged[i]);
	}
	putchar('\n');
#endif
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
	if (EOBPos != BLOCK_COLCNT * BLOCK_ROWCNT) {
		codeBuf[codePos].zeroCnt = 0;
		codeBuf[codePos].value = 0;
		codePos++;
	}
	*count = codePos;
}

BitString BitEncoder::getBitString(int val){
	if (val < 0) {
		val = -val;
		BitString bitString(val);
		bitString = ~bitString;
		return bitString;
	}
	return BitString(val);
}

BYTE BitEncoder::mergeToByte(int high4, int low4){
	return ((0xF0 & (BYTE)(high4 << 4)) | (0x0F & (BYTE)low4));
}

DPCM::DPCM() :pred(0){

}

void DPCM::reset(){
	pred = 0;
}

int DPCM::nextDiff(int val){
	int res = val - pred;
	pred = val;
	return res;
}
