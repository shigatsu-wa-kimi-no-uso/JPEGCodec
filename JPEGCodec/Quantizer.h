#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <cmath>
#include "typedef.h"
#include "UtilFunc.h"

class Quantizer
{
private:
	static BYTE _std_qtable_Y[8][8];

	static BYTE _std_qtable_C[8][8];

	static BYTE _user_qtable_Y[8][8];

	static BYTE _user_qtable_C[8][8];

	//缩放因子函数
	//    { 5000.0f/q 0<=q<50
	//s = { 200-2q    50<=q<100
	//    { 1         q=100
	static float _scalingFactor(BYTE stdQuant,float qualityFactor) {
		if (qualityFactor >= 0 && qualityFactor < 50) {
			return 5000.0f / qualityFactor;
		} else if (qualityFactor < 100) {
			return 200.0f - 2.0f * qualityFactor;
		} else {
			return 1;
		}
	}

	//量化因子计算公式
	static BYTE _userQuant(BYTE stdQuant, float qualityFactor) {
		return max((BYTE)(min(255.0f,(_scalingFactor(stdQuant,qualityFactor) * stdQuant + 50) / 100.0f)) ,1);
	}
	static void _generateUserTable(const BYTE(&stdTable)[8][8],BYTE(&userTable)[8][8], float qualityFactor) {
		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				userTable[r][c] = _userQuant(stdTable[r][c], qualityFactor);
			}
		}
	}


public:
	enum QTable{
		STD_QTABLE_LUMA,
		STD_QTABLE_CHROMA
	};

	static void generateUserTable(float qualityFactor) {
		_generateUserTable(_std_qtable_Y, _user_qtable_Y, qualityFactor);
		_generateUserTable(_std_qtable_C, _user_qtable_C, qualityFactor);
	}

	static const BYTE* getQuantTable(QTable tableType) {
		switch (tableType)
		{
		case STD_QTABLE_LUMA:
			return (BYTE*)_user_qtable_Y;
		case STD_QTABLE_CHROMA:
			return (BYTE*)_user_qtable_C;
		}
		return nullptr;
	}

	static void quantize(Block* input,Block* output,QTable qtableType) {
		static BYTE(*qtable)[8];
		switch (qtableType)
		{
		case STD_QTABLE_LUMA:
			qtable = _user_qtable_Y;
			break;
		case STD_QTABLE_CHROMA:
			qtable = _user_qtable_C;
		}

		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				(*output)[r][c] = myround((*input)[r][c] / qtable[r][c]);
			}
		}
	}
};

