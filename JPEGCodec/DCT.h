/*
* DCT.h
* Written by kiminouso, 2023/04/05
*/
#pragma once
#ifndef DCT_h__
#define DCT_h__
#include <array>
#include "typedef.h"
#include "UtilFunc.h"

class DCT
{
private:
	using dct_coeff_t = std::array<std::array<std::array<std::array<float, 8>, 8>, 8>, 8>;

	static constexpr dct_coeff_t _coeff{
		[]()consteval {
			std::array<std::array<float, 8>, 8> cosv{};
			std::array<std::array<std::array<std::array<float, 8>, 8>, 8>, 8> coeff{};
			for (int x = 0; x < 8; ++x) {
				for (int y = 0; y < 8; ++y) {
					cosv[x][y] = [&]() consteval {
						constexpr float cosval_table[9] = {
							1.0f,0.98078525f,0.9238795f,0.8314696f,0.70710677f,0.55557024f,0.38268343f,0.19509032f,0
						};
						bool isNegative = ((y * (2 * x + 1)) / 16) % 2;
						int phase = (y * (2 * x + 1)) % 16;
						float cosval = phase < 8 ? cosval_table[phase] : -cosval_table[16 - phase];
						return isNegative ? -cosval : cosval;
					}();
				}
			}
			auto c = [](int x)consteval { return x == 0 ? 0.35355338f : 0.5f; };
			for (int u = 0; u < 8; ++u) {
				for (int v = 0; v < 8; ++v) {
					for (int x = 0; x < 8; ++x) {
						for (int y = 0; y < 8; ++y) {
							coeff[u][v][x][y] = cosv[x][u] * cosv[y][v] * c(u) * c(v);
						}
					}
				}
			}
			return coeff;
		}()
	};

	static float _get_coeff(int u, int v, int x, int y);
	//1维DCT的表达式直接展开算法,利用重复计算的单元提取出来不重复计算
	static void _1D_8P_FDCT(const int(&seq)[BLOCK_COLCNT], int(&output)[BLOCK_COLCNT]);
	static void _1D_8P_IDCT(const int(&seq)[BLOCK_COLCNT], int(&output)[BLOCK_COLCNT]);
	static void _transpose(const Block& input, Block& output);
public:
	[[deprecated]]
	static void forwardDCT_BF(const Block& input, Block& output);
	[[deprecated]]
	static void inverseDCT_BF(const Block& input, Block& output);
	static void forwardDCT(const Block& input, Block& output);
	static void inverseDCT(const Block& input, Block& output);
};


#endif // DCT_h__