/*
* BitWriter.h
* Written by kiminouso, 2023/05/12
*/

#pragma once
#ifndef BitWriter_h__
#define BitWriter_h__
#include <vector>
#include "typedef.h"

class BitWriter
{
private:
	int _lastByteBitCnt;
	std::vector<BYTE>& _data;
	const BYTE _mask() const{
		return (1 << _lastByteBitCnt) - 1	;
	}
public:
	BitWriter(std::vector<BYTE>& data):_lastByteBitCnt(0),_data(data){}
	BitWriter(const BitWriter& src):_lastByteBitCnt(src._lastByteBitCnt),_data(src._data){}

	//��д��λ��д��λ
	void writeBit(const bool bit) {
		if (_lastByteBitCnt == 8) {
			//�����������:�����ֽ�Ϊ0xFFʱ,���0�Ժͱ��0xFFxx����
			if (_data.back() == 0xFF) {
				_data.push_back(0);
			}
			_data.push_back(0);
			_lastByteBitCnt = 0;
		}
		_data.back() |= ((BYTE)bit & 1) << (7 - _lastByteBitCnt);
		++_lastByteBitCnt;
	}

	void write(const BitString& src) {
		int len = src.length();
		for (int i = len - 1; i >= 0; --i) {
			writeBit(src.bit(i));
		}
	}

	void fillIncompleteByteWithOne() {
		while (_lastByteBitCnt != 8) {
			writeBit(1);
		}
	}
};

#endif // BitWriter_h__

