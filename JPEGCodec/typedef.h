#pragma once
#ifndef typedef_h__
#define typedef_h__
#define _CRT_SECURE_NO_WARNINGS
#include <type_traits>
#include <winsock.h>
#include "BitString.h"

#define MACROS
#define ALIAS
#define STRUCTS
#define DEBUG


#ifdef MACROS
#define BLOCK_COLCNT 8
#define BLOCK_ROWCNT 8
#define HUFFMAN_CODE_LENGTH_LIMIT 16
#define ALIGN(val,alignment_index)  (((val)+((1<<alignment_index) - 1)) & (~((1<<alignment_index) - 1)))
#endif

#ifdef ALIAS
using WORD = unsigned short;
using DWORD = unsigned long;
using LONG = long;
using BYTE = unsigned char;
using Block = float[BLOCK_ROWCNT][BLOCK_COLCNT];
#endif

#ifdef STRUCTS

#pragma pack(push,1)	//设置结构体为1字节对齐

union SubsampFact{
	enum : BYTE {
		YUV_444 = 0x11,
		YUV_440 = 0x12,
		YUV_422 = 0x21,
		YUV_420 = 0x22
	}factor;
	BYTE factor_v : 4;	//低4位 垂直采样因子
	BYTE factor_h : 4;	//高4位 水平采样因子
};

struct BitmapFileHeader
{
	WORD bfType;
	DWORD bfSize;	
	WORD bfReserved1; 
	WORD bfReserved2; 
	DWORD bfOffBits; 
};

struct BitmapInfoHeader
{
	DWORD biSize; 
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
};


struct RGBTriple
{
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
};

struct RGBQuad : RGBTriple
{
	BYTE rgbReserved;
};

template<typename T>
struct Sequence
{
private:
	T* _array;
public:
	const int length;
	Sequence(const int length) :length(length) {
		_array = new T[length];
	}
	T& operator[](const int index)const {
		return _array[index];
	}
	~Sequence() {
		delete[] _array;
	}
};

template<typename T>
struct Matrix
{
private:
	T** _2darr;
public:
	const int column_cnt;
	const int row_cnt;
	Matrix(){}
	Matrix(const int row_cnt, const int column_cnt)
		:column_cnt(column_cnt), row_cnt(row_cnt) {
		_2darr = new T * [row_cnt];
		for (int i = 0; i < row_cnt; ++i) {
			_2darr[i] = new T[column_cnt];
		}
	}

	
	template<int _col_cnt, int _row_cnt>
	Matrix(const T(&mat)[_row_cnt][_col_cnt]) 
		:column_cnt(_col_cnt), row_cnt(_row_cnt) {
		_2darr = new T * [row_cnt];
		for (int i = 0; i < row_cnt; ++i) {
			_2darr[i] = new T[column_cnt];
			for (int j = 0; j < column_cnt; ++j) {
				_2darr[i][j] = mat[i][j];
			}
		}
	}

	Matrix(std::initializer_list<std::initializer_list<T>> lists)
		:column_cnt(lists.size()), row_cnt(lists.begin()->size()) {
		_2darr = new T * [row_cnt];
		auto listsIter = lists.begin();
		for (int i = 0; i < row_cnt; ++i,++listsIter) {
			_2darr[i] = new T[column_cnt];
			auto listIter = listsIter->begin();
			for (int j = 0; j < column_cnt; ++j, ++listIter) {
				_2darr[i][j] = *listIter;
			}
		}
	}

	T* operator[](const int index) const {
		return _2darr[index];
	}

	T* row_pointer(const int index) const {
		return _2darr[index];
	}
	~Matrix() {
		if (_2darr == nullptr) {
			return;
		}
		for (int i = 0; i < row_cnt; ++i) {
			delete[] _2darr[i];
		}
		delete[] _2darr;
		_2darr = nullptr;
	}
};

struct YCbCr
{
	BYTE Y;
	BYTE Cb;
	BYTE Cr;
};



struct Marker 
{
	enum Common	: WORD
	{
		SOI = 0xFFD8,	//文件开始标识符
		EOI = 0xFFD9,	//文件结束标识符
		SOS = 0xFFDA,	//数据段扫描起始符
		DQT = 0xFFDB,	//量化表定义符
		DNL = 0xFFDC,	//number of lines definer
		DRI = 0xFFDD,	//restart interval definer
		DHP = 0xFFDE,	//hierarchical progression definer
		EXP = 0xFFDF,	//expand reference components
		COM = 0xFFFE,	//注释
		DAT_NIL=0xFF00	//数据段单字节为0
	};

	enum APP : WORD
	{
		APP0=0xFFE0,
		APP1,
		APP2,
		APP3,
		APP4,
		APP5,
		APP6,
		APP7,
		APP8,
		APP9,
		APP10,
		APP11,
		APP12,
		APP13,
		APP14,
		APP15
	};

	enum SOFMarker //帧起始符号, 包含差分/非差分式, 均采用哈夫曼编码方式
	{
		BASELINE_DCT = 0xFFC0,
		EXTENDED_SEQ_DCT = 0xFFC1,
		PROG_DCT = 0xFFC2,
		LOSSLESS = 0xFFC3,
		DIFF_SEQ_DCT = 0xFFC5,
		DIFF_PROG_DCT = 0xFFC6,
		DIFF_LOSSLESS = 0xFFC7,
	};

	enum HTSpec	//哈夫曼表定义符号
	{
		DHT = 0xFFC4
	};
};

//JFIF头
struct JPEG_JFIFHeader
{
	struct Info
	{
		WORD length;		//该段长度(除去marker的长度) length = 16 + 3*Xthumbnail*Ythumbnail
		char identifier[5] = "JFIF";	//标识符 "JFIF"的ASCII码, '\0'结尾
		BYTE version[2];	//版本 1.01(version[0]=1 version[1]=1) 或1.02(version[0]=1 version[1]=2) etc 
		enum Units : BYTE {
			NONE,	//无, 使用像素纵横比
			PPI,	//每英尺像素
			PPCM	//每厘米像素
		} units;	//密度单位		

		WORD Xdensity;		//水平分辨率
		WORD Ydensity;		//垂直分辨率(units为0时,Xdensity和Ydensity表示像素纵横比,通常取Xdensity=1,Ydensity=1)
		BYTE XThumbnail;	//缩略图的水平像素
		BYTE YThumbnail;	//虽略图的垂直像素
		//sizeof(JPEG_JFIFHeader::Info) == 16
	};

	WORD marker = Marker::APP0;		//起始标记
	Info info;						//内容
};

struct JPEG_QTableHeader
{
	struct Info
	{
		WORD length;		//长度 length = sizeof(Info) + 量化表长度
		BYTE tableID : 4;	//低4位 id
		enum PRECISION : BYTE
		{
			PREC_8BIT,
			PREC_16BIT
		}precision : 4;	//高4位 精度 0为8bit, 1为16bit
	};
	const WORD marker = Marker::DQT;	//定义符
	Info info;							//内容
};

struct JPEG_QTable_8BitPrec {
	JPEG_QTableHeader header;
	BYTE qtable_8bit[8][8];
};

struct JPEG_QTable_16BitPrec {
	JPEG_QTableHeader header;
	WORD qtable_16bit[8][8];
};


struct JPEG_FrameHeader_BDCT_YCbCr
{
	struct Info
	{
		WORD length;		//头长度
		const enum PRECISION : BYTE
		{
			PREC_12BIT,
			PREC_8BIT
		}precision = PREC_8BIT ;		//采样精度, 常见为8bit
		WORD numberOfLines;	//高度像素
		WORD samplesPerLine;//宽度像素
		const enum : BYTE {
			COMPCNT_GRAY,
			COMPCNT_YUV = 3,
			COMPCNT_CMYK = 4
		}numberOfComponents = COMPCNT_YUV;	//分量数(YCbCr为3, 只有灰度为1, CMYK为4)
		struct ImgComponent 
		{
			enum : BYTE 
			{
				LUMA = 0x1,
				CHROMA_B,
				CHROMA_R
			}identifier; //分量ID, YCbCr 3分量情况下, 1对应Y, 2对应Cb, 3对应Cr
			union {
				BYTE _subsampFact;
				BYTE VSubsampFact : 4;	//低4位 垂直采样因子
				BYTE HSubsampFact : 4;	//高4位 水平采样因子
			};
			BYTE qtableID;			//量化表ID
		} components[COMPCNT_YUV];			//YCbCr恒有3分量
	};
	const WORD marker = Marker::BASELINE_DCT;	//DCT类型标记
	Info info;									//内容
};



struct JPEG_HTableHeader
{
	struct Info
	{
		WORD length;	//长度
		BYTE tableID : 4;	//哈夫曼表ID, 低4位
		enum : BYTE
		{
			DC,
			AC
		}type : 4;				//哈夫曼表适用对象, 高4位, 有DC和AC可选
		BYTE tableEntryLen[16];	//哈夫曼表各项长度,共16项
	};
	const WORD marker = Marker::DHT;		//定义符
	Info info;								//内容
};

struct JPEG_HTable
{
	JPEG_HTableHeader header;
	BYTE* entries[16];		//哈夫曼表 16项,每项为一个数组
};

struct JPEG_ScanHeader_BDCT_YCbCr
{
	const WORD marker = Marker::SOS;		//定义符
	struct Info
	{
		WORD length;				//长度
		const enum : BYTE {
			COMPCNT_GRAY,
			COMPCNT_YUV = 3,
			COMPCNT_CMYK = 4
		}numberOfComponents = COMPCNT_YUV;	//分量数(YCbCr 为 3)
		struct ImgComponent
		{
			enum : BYTE
			{
				LUMA = 0x1,
				CHROMA_B,
				CHROMA_R
			}identifier; //分量ID, YCbCr 3分量情况下, 1对应Y, 2对应Cb, 3对应Cr; 与FrameHeader中的对应
			BYTE AC_HTableID : 4;
			BYTE DC_HTableID : 4;
		}components[COMPCNT_YUV];
		const BYTE startOfSpectralSel = 0;	//谱选择开始,固定为0
		const BYTE endOfSpectralSel = 63;	//谱选择结束,固定为63
		const BYTE successiveApproxBitPosL : 4 = 0;		//default initialization. C++20 only
		const BYTE successiveApproxBitPosH : 4 = 0;		//default initialization. C++20 only
	};
};

struct JPEG_Scan_4H
{
	JPEG_HTable HTables[4];
	JPEG_ScanHeader_BDCT_YCbCr scan;
	BYTE* compressedData;
};

struct JPEG_Frame_BDCT_2Q4H
{
	JPEG_JFIFHeader jfifHeader;
	JPEG_QTable_8BitPrec QTables[2];
	JPEG_FrameHeader_BDCT_YCbCr frameHeader;
	JPEG_Scan_4H scanData;
};

//Baseline DCT 2量化表 4哈夫曼表 JPEG 文件结构
struct JPEG_File_BDCT_2Q4H
{
	const WORD SOImarker = Marker::SOI;
	JPEG_Frame_BDCT_2Q4H frame;
	const WORD EOFmarker = Marker::EOI;
};


struct MCU {
	Block** y;
	Block* cb;
	Block* cr;
};

struct RLECode {
	int zeroCnt;
	int value;
};

#endif

#pragma pack(pop)

#endif // typedef_h__