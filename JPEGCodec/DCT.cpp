#include "DCT.h"

float DCT::_get_coeff(int u, int v, int x, int y) {
	return _coeff[u][v][x][y];
}

//1维DCT的表达式直接展开算法,利用重复计算的单元提取出来不重复计算,降低时间复杂度
inline void DCT::_1D_8P_FDCT(const int(&seq)[BLOCK_COLCNT], int(&output)[BLOCK_COLCNT]) {
	constexpr double PI = 3.141592653589793;
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

	output[0] = myround(0.5*(c0x0p7 + c0x1p6 + c0x2p5 + c0x3p4));
	output[1] = myround(0.5*(c1x0n7 + c3x1n6 + c5x2n5 + c7x3n4));
	output[2] = myround(0.5*(c2x0p7 + c6x1p6 - c6x2p5 - c2x3p4));
	output[3] = myround(0.5*(c3x0n7 - c7x1n6 - c1x2n5 - c5x3n4));
	output[4] = myround(0.5*(c0x0p7 - c0x1p6 - c0x2p5 + c0x3p4));
	output[5] = myround(0.5*(c5x0n7 - c1x1n6 + c7x2n5 + c3x3n4));
	output[6] = myround(0.5*(c6x0p7 - c2x1p6 + c2x2p5 - c6x3p4));
	output[7] = myround(0.5*(c7x0n7 - c5x1n6 + c3x2n5 - c1x3n4));
}

void DCT::_1D_8P_IDCT(const int(&seq)[BLOCK_COLCNT], int(&output)[BLOCK_COLCNT]){
	constexpr double PI = 3.141592653589793;
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

	const double c0f0 = c[0] * seq[0];

	const double c1f1 = c[1] * seq[1];
	const double c3f1 = c[3] * seq[1];
	const double c5f1 = c[5] * seq[1];
	const double c7f1 = c[7] * seq[1];

	const double c2f2 = c[2] * seq[2];
	const double c6f2 = c[2] * seq[2];

	const double c1f3 = c[1] * seq[3];
	const double c3f3 = c[3] * seq[3];
	const double c5f3 = c[5] * seq[3];
	const double c7f3 = c[7] * seq[3];

	const double c4f4 = c[4] * seq[4];

	const double c1f5 = c[1] * seq[5];
	const double c3f5 = c[3] * seq[5];
	const double c5f5 = c[5] * seq[5];
	const double c7f5 = c[7] * seq[5];

	const double c2f6 = c[2] * seq[6];
	const double c6f6 = c[6] * seq[6];

	const double c1f7 = c[1] * seq[7];
	const double c3f7 = c[3] * seq[7];
	const double c5f7 = c[5] * seq[7];
	const double c7f7 = c[7] * seq[7];

	output[0] = myround(0.5 * (c0f0 + c1f1 + c2f2 + c3f3 + c4f4 + c5f5 + c6f6 + c7f7));
	output[1] = myround(0.5 * (c0f0 + c3f1 + c6f2 - c7f3 - c4f4 - c1f5 - c2f6 - c5f7));
	output[2] = myround(0.5 * (c0f0 + c5f1 - c6f2 - c1f3 - c4f4 + c7f5 + c2f6 + c3f7));
	output[3] = myround(0.5 * (c0f0 + c7f1 - c2f2 - c5f3 + c4f4 + c3f5 - c6f6 - c1f7));
	output[4] = myround(0.5 * (c0f0 - c7f1 - c2f2 + c5f3 + c4f4 - c3f5 - c6f6 + c1f7));
	output[5] = myround(0.5 * (c0f0 - c5f1 - c6f2 + c1f3 - c4f4 - c7f5 + c2f6 - c3f7));
	output[6] = myround(0.5 * (c0f0 - c3f1 + c6f2 + c7f3 - c4f4 + c1f5 - c2f6 + c5f7));
	output[7] = myround(0.5 * (c0f0 - c1f1 + c2f2 - c3f3 + c4f4 - c5f5 + c6f6 - c7f7));
}

void DCT::_transpose(const Block& input, Block& output) {
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		for (int j = 0; j < BLOCK_COLCNT; ++j) {
			output[j][i] = input[i][j];
		}
	}
}

void DCT::forwardDCT(const Block& input, Block& output){
	Block tmpBlock;
	Block tmpBlock2;
	//2维DCT拆解为2次1维DCT
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		_1D_8P_FDCT(input[i], tmpBlock[i]);
	}
	_transpose(tmpBlock, tmpBlock2);
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		_1D_8P_FDCT(tmpBlock2[i], tmpBlock[i]);
	}
	_transpose(tmpBlock, output);
}

[[deprecated]]
void DCT::forwardDCT_BF(const Block& input, Block& output) {
	//暴力算法
	for (int u = 0; u < BLOCK_ROWCNT; ++u) {
		for (int v = 0; v < BLOCK_COLCNT; ++v) {
			float t = 0;
			for (int x = 0; x < BLOCK_ROWCNT; ++x) {
				for (int y = 0; y < BLOCK_COLCNT; ++y) {
					t += input[x][y] * _get_coeff(u, v, x, y);
				}
			}
			output[u][v] = myround(t);
		}
	}
}

[[deprecated]]
void DCT::inverseDCT_BF(const Block& input, Block& output){
	//暴力算法
	for (int x = 0; x < BLOCK_ROWCNT; ++x) {
		for (int y = 0; y < BLOCK_COLCNT; ++y) {
			float t = 0;
			for (int u = 0; u < BLOCK_ROWCNT; ++u) {
				for (int v = 0; v < BLOCK_COLCNT; ++v) {
					t += input[u][v] * _get_coeff(u, v, x, y);
				}
			}
			output[x][y] = myround(t);
		}
	}
}

void DCT::inverseDCT(const Block& input, Block& output){
	Block tmpBlock;
	Block tmpBlock2;
	//2维IDCT拆解为2次1维IDCT
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		_1D_8P_IDCT(input[i], tmpBlock[i]);
	}
	_transpose(tmpBlock, tmpBlock2);
	for (int i = 0; i < BLOCK_ROWCNT; ++i) {
		_1D_8P_IDCT(tmpBlock2[i], tmpBlock[i]);
	}
	_transpose(tmpBlock, output);
}
