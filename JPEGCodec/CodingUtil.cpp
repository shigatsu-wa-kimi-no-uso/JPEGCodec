/*
* CodingUtil.cpp
* Written by kiminouso, 2023/04/06
*/
#include "CodingUtil.h"
#include "UtilFunc.h"

void RunLengthCodec::encode(const int* seq,const int seqLen, RLCode* codeBuf, int* count){
	int zeroCnt = 0;
	int codePos = 0;
	//找到EOB的位置
	int EOBPos;
	for (EOBPos = seqLen - 1; EOBPos >= 0; --EOBPos) {
		if (seq[EOBPos] != 0) {
			EOBPos++;
			break;
		}
	}
	//RLE
	for (int i = 0; i < EOBPos; ++i) {
		int curr = seq[i];
		if (zeroCnt != 15 && curr == 0) {
			zeroCnt++;
		} else {
			codeBuf[codePos].zeroCnt = zeroCnt;
			codeBuf[codePos].value = curr;
			codePos++;
			zeroCnt = 0;
		}
	}
	if (EOBPos != seqLen) {
		codeBuf[codePos].zeroCnt = 0;
		codeBuf[codePos].value = 0;
		codePos++;
	}
	*count = codePos;
}


void RunLengthCodec::decode(const RLCode* codes, const int codeCnt, const int seqLength,int* seqBuf){
	int i, pos = 0;
	for (i = 0; i < codeCnt - 1; ++i) {
		for (int j = 0; j < codes[i].zeroCnt; ++j) {
			seqBuf[pos++] = 0;
		}
		seqBuf[pos++] = codes[i].value;
	}
	//EOB判断
	if (codes[i].value == 0 && codes[i].zeroCnt == 0) {
		while (pos != seqLength) {
			seqBuf[pos++] = 0;
		}
	} else {
		for (int j = 0; j < codes[i].zeroCnt; ++j) {
			seqBuf[pos++] = 0;
		}
		seqBuf[pos++] = codes[i].value;
	}
}

BitString BitCodec::getBitString(int val) {
	if (val < 0) {
		val = -val;
		BitString bitString(val);
		bitString = ~bitString;
		return bitString;
	}
	return BitString(val);
}

BYTE BitCodec::mergeToByte(int high4, int low4) {
	return ((0xF0 & (BYTE)(high4 << 4)) | (0x0F & (BYTE)low4));
}

int BitCodec::getValue(const BitString& bitString) {
	//负数时,最高位为0
	if (bitString.bit(bitString.length() - 1) == 0) {
		return -((int)(~bitString).value());
	} else {
		return bitString.value();
	}
}

DPCM::DPCM() :_preVal(0){
}

void DPCM::reset(){
	_preVal = 0;
}

int DPCM::nextDiff(int val){
	int res = val - _preVal;
	_preVal = val;
	return res;
}

int DPCM::nextVal(int diff){
	int res = diff + _preVal;
	_preVal = res;
	return res;
}
