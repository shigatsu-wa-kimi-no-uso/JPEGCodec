#pragma once
#include <cmath>
#include "typedef.h"
#include "UtilFunc.h"

class Quantizer
{
private:
	static const BYTE _std_qtable_Y[8][8];

	static const BYTE _std_qtable_C[8][8];

	static BYTE _user_qtable_Y[8][8];

	static BYTE _user_qtable_C[8][8];

	//缩放因子函数
	//    { 5000.0f/q 0<=q<50
	//s = { 200-2q    50<=q<100
	//    { 1         q=100
	static float _scalingFactor(BYTE stdQuant,float qualityFactor);

	//量化因子计算公式
	static BYTE _userQuant(BYTE stdQuant, float qualityFactor);

	static void _generateUserTable(const BYTE(&stdTable)[8][8],BYTE(&userTable)[8][8], float qualityFactor);

public:
	enum QTable{
		STD_QTABLE_LUMA,
		STD_QTABLE_CHROMA
	};

	static void generateUserTable(float qualityFactor);

	static const BYTE (*getQuantTable(QTable tableType))[BLOCK_ROWCNT][BLOCK_COLCNT];

	static void quantize(const Block& input, const QTable tableType,Block& output);

	static void dequantize(const Block& input, const BYTE(&quantTable)[BLOCK_ROWCNT][BLOCK_COLCNT], Block& output);
};

