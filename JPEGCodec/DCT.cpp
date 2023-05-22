#include "DCT.h"

float DCT::_get_coeff(int u, int v, int x, int y) {
	return _coeff[u][v][x][y];
}

inline float DCT::_shift128(float val) {
	return val - 128;
}

//1维DCT的表达式直接展开算法,利用重复计算的单元提取出来不重复计算,降低时间复杂度
inline void DCT::_1D_8P_DCT(const int(&seq)[BLOCK_COLCNT], int(&output)[BLOCK_COLCNT]) {
	constexpr double PI = 3.141592653;
	const static double c[8] = {
		1.0 / sqrt(2),
		cos(PI * 1 / 16.0),
		cos(PI * 2 / 16.0),
		cos(PI * 3 / 16.0),
		cos(PI * 4 / 16.0),
		cos(PI * 5 / 16.0),
		cos(PI * 6 / 16.0),
		cos(PI * 7 / 16.0)
	};

	const int x0p7 = seq[0] + seq[7];
	const int x1p6 = seq[1] + seq[6];
	const int x2p5 = seq[2] + seq[5];
	const int x3p4 = seq[3] + seq[4];
	const int x0n7 = seq[0] - seq[7];
	const int x1n6 = seq[1] - seq[6];
	const int x2n5 = seq[2] - seq[5];
	const int x3n4 = seq[3] - seq[4];

	const double c0x0p7 = c[0] * x0p7;
	const double c0x1p6 = c[0] * x1p6;
	const double c0x2p5 = c[0] * x2p5;
	const double c0x3p4 = c[0] * x3p4;
	const double c0x0n7 = c[0] * x0n7;
	const double c0x1n6 = c[0] * x1n6;
	const double c0x2n5 = c[0] * x2n5;
	const double c0x3n4 = c[0] * x3n4;

	const double c1x0n7 = c[1] * x0n7;
	const double c1x1n6 = c[1] * x1n6;
	const double c1x2n5 = c[1] * x2n5;
	const double c1x3n4 = c[1] * x3n4;

	const double c2x0p7 = c[2] * x0p7;
	const double c2x1p6 = c[2] * x1p6;
	const double c2x2p5 = c[2] * x2p5;
	const double c2x3p4 = c[2] * x3p4;

	const double c3x0p7 = c[3] * x0p7;
	const double c3x0n7 = c[3] * x0n7;
	const double c3x1p6 = c[3] * x1p6;
	const double c3x1n6 = c[3] * x1n6;
	const double c3x2p5 = c[3] * x2p5;
	const double c3x2n5 = c[3] * x2n5;
	const double c3x3p4 = c[3] * x3p4;
	const double c3x3n4 = c[3] * x3n4;

	const double c5x0n7 = c[5] * x0n7;
	const double c5x1n6 = c[5] * x1n6;
	const double c5x2n5 = c[5] * x2n5;
	const double c5x3n4 = c[5] * x3n4;

	const double c6x0p7 = c[6] * x0p7;
	const double c6x0n7 = c[6] * x0n7;
	const double c6x1p6 = c[6] * x1p6;
	const double c6x1n6 = c[6] * x1n6;
	const double c6x2p5 = c[6] * x2p5;
	const double c6x2n5 = c[6] * x2n5;
	const double c6x3p4 = c[6] * x3p4;
	const double c6x3n4 = c[6] * x3n4;

	const double c7x0n7 = c[7] * x0n7;
	const double c7x1n6 = c[7] * x1n6;
	const double c7x2n5 = c[7] * x2n5;
	const double c7x3n4 = c[7] * x3n4;

	/*
	output[0] = 0.5 * (c0x0p7 + c0x1p6 + c0x2p5 + c0x3p4);
	output[1] = 0.5 * (c1x0n7 + c3x1n6 + c5x2n5 + c7x3n4);
	output[2] = 0.5 * (c2x0p7 + c6x1p6 - c6x2p5 - c2x3p4);
	output[3] = 0.5 * (c3x0n7 - c7x1n6 - c1x2n5 - c5x3n4);
	output[4] = 0.5 * (c0x0p7 - c0x1p6 - c0x2p5 + c0x3p4);
	output[5] = 0.5 * (c5x0n7 - c1x1n6 + c7x2n5 + c3x3n4);
	output[6] = 0.5 * (c6x0p7 - c2x1p6 + c2x2p5 - c6x3p4);
	output[7] = 0.5 * (c7x0n7 - c5x1n6 + c3x2n5 - c1x3n4);
	*/
	output[0] = myround(0.5*(c0x0p7 + c0x1p6 + c0x2p5 + c0x3p4));
	output[1] = myround(0.5*(c1x0n7 + c3x1n6 + c5x2n5 + c7x3n4));
	output[2] = myround(0.5*(c2x0p7 + c6x1p6 - c6x2p5 - c2x3p4));
	output[3] = myround(0.5*(c3x0n7 - c7x1n6 - c1x2n5 - c5x3n4));
	output[4] = myround(0.5*(c0x0p7 - c0x1p6 - c0x2p5 + c0x3p4));
	output[5] = myround(0.5*(c5x0n7 - c1x1n6 + c7x2n5 + c3x3n4));
	output[6] = myround(0.5*(c6x0p7 - c2x1p6 + c2x2p5 - c6x3p4));
	output[7] = myround(0.5*(c7x0n7 - c5x1n6 + c3x2n5 - c1x3n4));
}

inline void DCT::_transpose(const Block& input, Block& output) {
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		for (int j = 0; j < BLOCK_COLCNT; ++j) {
			output[j][i] = input[i][j];
		}
	}
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
				double t = 0;
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

void DCT::forwardDCT(const Block& input, Block& output){
	
	Block tmpBlock;
	Block tmpBlock2;
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		_1D_8P_DCT(input[i], tmpBlock[i]);
	}
	_transpose(tmpBlock, tmpBlock2);
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		_1D_8P_DCT(tmpBlock2[i], tmpBlock[i]);
	}
	_transpose(tmpBlock, output);

	/*
	for (int u = 0; u < BLOCK_ROWCNT; ++u) {
		for (int v = 0; v < BLOCK_COLCNT; ++v) {
			double t = 0;
			for (int x = 0; x < BLOCK_ROWCNT; ++x) {
				for (int y = 0; y < BLOCK_COLCNT; ++y) {
					t += input[x][y] * _get_coeff(u, v, x, y);
				}
			}
			output[u][v] = t;
		}
	}*/
}

