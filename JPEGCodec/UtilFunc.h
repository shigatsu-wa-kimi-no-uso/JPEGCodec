/*
* UtilFunc.h
* Written by kiminouso, 2023/05/09
*/
#pragma once
#ifndef UtilFunc_h__
#define UtilFunc_h__
#include <winsock.h>
#include "typedef.h"
#pragma comment(lib,"ws2_32.lib")

template<typename Tx, typename Ty>
inline void getZigzaggedSequence(const Tx (&seq)[BLOCK_ROWCNT][BLOCK_COLCNT], Ty(&zigzaggedSeq)[BLOCK_COLCNT * BLOCK_ROWCNT]) {
	static DWORD zigzag[8][8] = {
			0,1,5,6,14,15,27,28,
			2,4,7,13,16,26,29,42,
			3,8,12,17,25,30,41,43,
			9,11,18,24,31,40,44,53,
			10,19,23,32,39,45,52,54,
			20,22,33,38,46,51,55,60,
			21,34,37,47,50,56,59,61,
			35,36,48,49,57,58,62,63
	};
	for (DWORD r = 0; r < BLOCK_ROWCNT; ++r) {
		for (DWORD c = 0; c < BLOCK_COLCNT; ++c) {
			zigzaggedSeq[zigzag[r][c]] = seq[r][c];
		}
	}
}

inline const char* getoptarg(int argc, char** argv, const char* opt) {
	for (int i = 0; i < argc - 1; ++i) {
		if (strcmp(argv[i], opt) == 0) {
			return argv[i + 1];
		}
	}
	return nullptr;
}

inline bool testopt(int argc, char** argv, const char* opt) {
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], opt) == 0) {
			return true;
		}
	}
	return false;
}


inline DWORD big_endian(DWORD val) {
	return htonl(val);
}

inline WORD big_endian(WORD val) {
	return htons(val);
}

inline DWORD host_order(DWORD val) {
	return ntohl(val);
}

inline WORD host_order(WORD val) {
	return ntohs(val);
}

inline int myround(float val) {
	int intval = (int)val;
	if (val - intval >= 0.5f) {
		return intval + 1;
	} else {
		return intval;
	}
}


#endif // UtilFunc_h__