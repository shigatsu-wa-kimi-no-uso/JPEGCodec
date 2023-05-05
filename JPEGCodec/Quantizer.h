#pragma once
#include "typedef.h"
#include <cmath>





class Quantizer
{
private:
	static DWORD _std_qtable_Y[8][8];

	static DWORD _std_qtable_C[8][8];

	Matrix<float>* _blocks;

	DWORD _blockCnt;

	DWORD (*_qtable)[8];

public:
	void setBlocks(Matrix<float>* blocks,DWORD blockCnt) {
		_blocks = blocks;
		_blockCnt = blockCnt;
	}
	enum QTable{
		STD_QTABLE_LUMA,
		STD_QTABLE_CHROMA
	};

	void setQTable(QTable mode) {
		switch (mode)
		{
		case STD_QTABLE_LUMA:
			_qtable = _std_qtable_Y;
			break;
		case STD_QTABLE_CHROMA:
			_qtable = _std_qtable_C;
		}
	}
	static void quantize(Block* input,Block* output,QTable qtableType) {
		static DWORD (*qtable)[8];
		switch (qtableType)
		{
		case STD_QTABLE_LUMA:
			qtable = _std_qtable_Y;
			break;
		case STD_QTABLE_CHROMA:
			qtable = _std_qtable_C;
		}

		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				(*output)[r][c] = myround((*input)[r][c] / qtable[r][c]);
			}
		}
	}
	void quantize(Matrix<float>* outputSequence) {
		for (int i = 0; i < _blockCnt; ++i) {
			for (int x = 0; x < 8; ++x) {
				for (int y = 0; y < 8; ++y) {
					outputSequence[i][x][y] = myround(_blocks[i][x][y] / _qtable[x][y]);
				}
			}
		}
	}

};

