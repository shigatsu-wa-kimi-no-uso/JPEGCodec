/*
* UtilFunc.h
* Written by kiminouso, 2023/05/09
*/
#pragma once
#ifndef UtilFunc_h__
#define UtilFunc_h__
#include "typedef.h"



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
	int intval = val;
	if (val - intval >= 0.5f) {
		return intval + 1;
	} else {
		return intval;
	}
}

#endif // UtilFunc_h__