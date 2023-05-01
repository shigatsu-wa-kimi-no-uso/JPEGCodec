#pragma once
#include "typedef.h"
#include <cmath>





class Quantizer
{
private:
	static DWORD _std_qtable_Y[8][8];

	static DWORD _std_qtable_C[8][8];

	static DWORD _zigzag[8][8];

	Matrix<float>* _blocks;

	DWORD _blockCnt;

	DWORD (*_qtable)[8];

public:
	void setBlocks(Matrix<float>* blocks,DWORD blockCnt) {
		_blocks = blocks;
		_blockCnt = blockCnt;
	}
	enum Mode{
		STD_QTABLE_LUMA,
		STD_QTABLE_CHROMA
	};

	void setQTable(Mode mode) {
		switch (mode)
		{
		case STD_QTABLE_LUMA:
			_qtable = _std_qtable_Y;
			break;
		case STD_QTABLE_CHROMA:
			_qtable = _std_qtable_C;
		}
	}

	void quantize(Matrix<DWORD>* outputSequence) {
		for (int i = 0; i < _blockCnt; ++i) {
			for (int x = 0; x < 8; ++x) {
				for (int y = 0; y < 8; ++y) {
					outputSequence[i][x][y] = (DWORD)std::round(_blocks[i][x][y] / _qtable[x][y]);
				}
			}
		}
	}

};

