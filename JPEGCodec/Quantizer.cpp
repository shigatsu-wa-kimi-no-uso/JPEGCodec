#include "Quantizer.h"

const BYTE Quantizer::_std_qtable_Y[8][8] = {
		16,11,10,16,24,40,51,61,
		12,12,14,19,26,58,60,55,
		14,13,16,24,40,57,69,56,
		14,17,22,29,51,87,80,62,
		18,22,37,56,68,109,103,77,
		24,35,55,64,81,104,113,92,
		49,64,78,87,103,121,120,101,
		72,92,95,98,112,100,103,99
};

const BYTE Quantizer::_std_qtable_C[8][8] = {
	17,18,24,47,99,99,99,99,
	18,21,26,66,99,99,99,99,
	24,26,56,99,99,99,99,99,
	47,66,99,99,99,99,99,99,
	99,99,99,99,99,99,99,99,
	99,99,99,99,99,99,99,99,
	99,99,99,99,99,99,99,99,
	99,99,99,99,99,99,99,99
};

BYTE Quantizer::_user_qtable_Y[8][8];

BYTE Quantizer::_user_qtable_C[8][8];

float Quantizer::_scalingFactor(BYTE stdQuant, float qualityFactor){
	if (qualityFactor >= 0 && qualityFactor < 50) {
		return 5000.0f / qualityFactor;
	} else if (qualityFactor < 100) {
		return 200.0f - 2.0f * qualityFactor;
	} else {
		return 1;
	}
}

BYTE Quantizer::_userQuant(BYTE stdQuant, float qualityFactor){
	return max((BYTE)(min(255.0f, (_scalingFactor(stdQuant, qualityFactor) * stdQuant + 50.0f) / 100.0f)), 1);
}

void Quantizer::_generateUserTable(const BYTE(&stdTable)[8][8], BYTE(&userTable)[8][8], float qualityFactor){
	for (int r = 0; r < BLOCK_ROWCNT; ++r) {
		for (int c = 0; c < BLOCK_COLCNT; ++c) {
			userTable[r][c] = _userQuant(stdTable[r][c], qualityFactor);
		}
	}
}

void Quantizer::generateUserTable(float qualityFactor){
	_generateUserTable(_std_qtable_Y, _user_qtable_Y, qualityFactor);
	_generateUserTable(_std_qtable_C, _user_qtable_C, qualityFactor);
}

const BYTE (*Quantizer::getQuantTable(QTable tableType))[BLOCK_ROWCNT][BLOCK_COLCNT]{
	switch (tableType)
	{
	case STD_QTABLE_LUMA:
		return &_user_qtable_Y;
	case STD_QTABLE_CHROMA:
		return &_user_qtable_C;
	}
	return nullptr;
}

void Quantizer::quantize(const Block& input,const QTable tableType, Block& output){
	static BYTE(*qtable)[8];
	switch (tableType)
	{
	case STD_QTABLE_LUMA:
		qtable = _user_qtable_Y;
		break;
	case STD_QTABLE_CHROMA:
		qtable = _user_qtable_C;
	}

	for (int r = 0; r < BLOCK_ROWCNT; ++r) {
		for (int c = 0; c < BLOCK_COLCNT; ++c) {
			output[r][c] = myround(input[r][c]*1.0 / qtable[r][c]);
		}
	}
}

void Quantizer::dequantize(const Block& input, const BYTE(&quantTable)[BLOCK_ROWCNT][BLOCK_COLCNT], Block& output) {
	for (int r = 0; r < BLOCK_ROWCNT; ++r) {
		for (int c = 0; c < BLOCK_COLCNT; ++c) {
			output[r][c] = myround(input[r][c] * 1.0 * quantTable[r][c]);
		}
	}
}