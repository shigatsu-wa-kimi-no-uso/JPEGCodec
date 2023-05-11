#include "DCT.h"

float DCT::_get_coeff(int u, int v, int x, int y) {
	return _coeff[u][v][x][y];
}

inline float DCT::_shift128(float val) {
	return val - 128;
}

void DCT::setBlocks(Matrix<float>* dctblocks, DWORD blockCnt) {
	_dctblocks = dctblocks;
	_blockCnt = blockCnt;
}

void DCT::transform(Matrix<float>* outputBlocks) {
	for (DWORD i = 0; i < _blockCnt; ++i) {
		for (int u = 0; u < 8; ++u) {
			for (int v = 0; v < 8; ++v) {
				//printf("F(%d,%d)=", u, v);
				float t = 0;
				for (int x = 0; x < 8; ++x) {
					for (int y = 0; y < 8; ++y) {
						//t += mat[x][y] * cosine(x, u) * cosine(y, v) * c(u) * c(v);
						t += _shift128(_dctblocks[i][x][y]) * _get_coeff(u, v, x, y);
						//printf("%.3f*%.3f + ", _dctblocks[i][x][y], _get_coeff(u, v, x, y));

					}/*printf("\n");*/
				}
				outputBlocks[i][u][v] = t;
				//printf(" = %.3f\n\n", t);
			}
		}
	}

}

void DCT::forwardDCT(Block* input, Block* output)
{
	for (int u = 0; u < BLOCK_ROWCNT; ++u) {
		for (int v = 0; v < BLOCK_COLCNT; ++v) {
			float t = 0;
			for (int x = 0; x < BLOCK_ROWCNT; ++x) {
				for (int y = 0; y < BLOCK_COLCNT; ++y) {
					t += input[0][x][y] * _get_coeff(u, v, x, y);
				}
			}
			output[0][u][v] = t;
		}
	}
}
