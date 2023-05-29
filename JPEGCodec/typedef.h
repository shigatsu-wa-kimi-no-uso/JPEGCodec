/*
* typedef.h
* Written by kiminouso, 2023/04/05
*/
#pragma once
#ifndef typedef_h__
#define typedef_h__
#include <vector>
#include <assert.h>
#define MACROS
#define ALIAS
#define STRUCTS

#ifdef MACROS
#define BLOCK_COLCNT 8
#define BLOCK_ROWCNT 8
#define HUFFMAN_CODE_LENGTH_LIMIT 16
#define ALIGN(val,alignment_index)  (((val)+((((1)<<(alignment_index))) - 1)) & (~((((1)<<(alignment_index))) - 1)))
#endif

#ifdef ALIAS
using size_t = unsigned long long;
using WORD = unsigned short;
using DWORD = unsigned long;
using LONG = long;
using BYTE = unsigned char;
using Block = int[BLOCK_ROWCNT][BLOCK_COLCNT];
using HuffmanTable = std::vector<std::vector<DWORD>>;
#endif

#ifdef STRUCTS

#pragma pack(push,1)	//设置结构体为1字节对齐

enum class HTableType : BYTE
{
	DC,
	AC,
	MAXENUMVAL
};

enum class Component : BYTE
{
	LUMA = 0x1,
	CHROMA_B,
	CHROMA_R,
	MAXENUMVAL
};

union SubsampFact {
	BYTE rawVal;
	enum : BYTE {
		YUV_444 = 0x11,
		YUV_440 = 0x12,
		YUV_422 = 0x21,
		YUV_420 = 0x22
	}factor;
	struct {
		BYTE factor_v : 4;	//低4位 垂直采样因子
		BYTE factor_h : 4;	//高4位 水平采样因子
	};
};

struct ComponentConfig {
	int QTableSel;
	int DC_HTableSel;
	int AC_HTableSel;
	SubsampFact subsampFact;
};

struct BitCode {
	enum : BYTE {
		EOB = 0
	};
	union {
		struct {
			BYTE bitLength : 4;
			BYTE zeroCnt : 4;
		};
		BYTE codedUnit;
	};
	DWORD bits;
};

#ifdef ALIAS
using BitCodeArray = std::vector<BitCode>;
#endif

struct BitCodeUnit {
	std::vector<BitCodeArray> y;
	BitCodeArray cb;
	BitCodeArray cr;
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
	const DWORD column_cnt;
	const DWORD row_cnt;
	Matrix(){}
	Matrix(const DWORD row_cnt, const DWORD column_cnt)
		:column_cnt(column_cnt), row_cnt(row_cnt) {
		_2darr = new T * [row_cnt];
		for (DWORD i = 0; i < row_cnt; ++i) {
			_2darr[i] = new T[column_cnt];
		}
	}

	
	template<DWORD _col_cnt, DWORD _row_cnt>
	Matrix(const T(&mat)[_row_cnt][_col_cnt]) 
		:column_cnt(_col_cnt), row_cnt(_row_cnt) {
		_2darr = new T * [row_cnt];
		for (DWORD i = 0; i < row_cnt; ++i) {
			_2darr[i] = new T[column_cnt];
			for (DWORD j = 0; j < column_cnt; ++j) {
				_2darr[i][j] = mat[i][j];
			}
		}
	}

	Matrix(std::initializer_list<std::initializer_list<T>> lists)
		:column_cnt(lists.size()), row_cnt(lists.begin()->size()) {
		_2darr = new T * [row_cnt];
		auto listsIter = lists.begin();
		for (DWORD i = 0; i < row_cnt; ++i,++listsIter) {
			_2darr[i] = new T[column_cnt];
			auto listIter = listsIter->begin();
			for (DWORD j = 0; j < column_cnt; ++j, ++listIter) {
				_2darr[i][j] = *listIter;
			}
		}
	}

	T* operator[](const DWORD index) const {
		assert(index < row_cnt);
		return _2darr[index];
	}

	T* row_pointer(const DWORD index) const {
		return _2darr[index];
	}
	~Matrix() {
		if (_2darr == nullptr) {
			return;
		}
		for (DWORD i = 0; i < row_cnt; ++i) {
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

//不含0xFF前缀
struct Marker
{
	enum Common	: BYTE
	{
		SOI = 0xD8,	//文件开始标识符
		EOI = 0xD9,	//文件结束标识符
		SOS = 0xDA,	//数据段扫描起始符
		DQT = 0xDB,	//量化表定义符
		DNL = 0xDC,	//number of lines definer
		DRI = 0xDD,	//restart interval definer
		DHP = 0xDE,	//hierarchical progression definer
		EXP = 0xDF,	//expand reference components
		COM = 0xFE,	//注释
		ESC = 0xFF, //转义符(所有marker前的0xFF前缀)
		DAT_NIL=0x00	//数据段单字节为0
	};

	enum APP : BYTE
	{
		APP0 = 0xE0,
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

	enum SOFMarker : BYTE //帧起始符号, 包含差分/非差分式, 均采用哈夫曼编码方式
	{
		BASELINE_DCT = 0xC0,
		EXTENDED_SEQ_DCT = 0xC1,
		PROG_DCT = 0xC2,
		LOSSLESS = 0xC3,
		DIFF_SEQ_DCT = 0xC5,
		DIFF_PROG_DCT = 0xC6,
		DIFF_LOSSLESS = 0xC7,
	};

	enum HTSpec : BYTE	//哈夫曼表定义符号
	{
		DHT = 0xC4
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
	BYTE esc;
	BYTE JFIFmarker;				//起始标记
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
	BYTE esc;
	BYTE DQTmarker;	//定义符Marker::DQT
	Info info;				//内容
};

struct JPEG_QTable_8BitPrec {
	JPEG_QTableHeader header;
	BYTE qtable_8bit[8][8];
};

struct JPEG_QTable_16BitPrec {
	JPEG_QTableHeader header;
	WORD qtable_16bit[8][8];
};

struct JPEG_FrameHeader_YCbCr
{
	struct Info
	{
		WORD length;		//头长度
		enum PRECISION : BYTE
		{
			PREC_12BIT = 12,
			PREC_8BIT = 8
		}precision;		//采样精度, 常见为8bit
		WORD numberOfLines;	//高度像素
		WORD samplesPerLine;//宽度像素
		enum : BYTE {
			COMPCNT_GRAY,
			COMPCNT_YUV = 3,
			COMPCNT_CMYK = 4
		}numberOfComponents;	//分量数(YCbCr为3, 只有灰度为1, CMYK为4)
		struct ImgComponent 
		{
			Component identifier; //分量ID, YCbCr 3分量情况下, 1对应Y, 2对应Cb, 3对应Cr
			union {
				BYTE subsampFact;
				struct {
					BYTE VSubsampFact : 4;	//低4位 垂直采样因子
					BYTE HSubsampFact : 4;	//高4位 水平采样因子
				};
			};
			BYTE qtableID;			//量化表ID
		} components[COMPCNT_YUV];			//YCbCr恒有3分量
	};
	BYTE esc;
	BYTE DCTmarker;	//DCT类型标记 Marker::BASELINE_DCT
	Info info;				//内容
};

struct JPEG_HTableHeader
{
	struct Info
	{
		WORD length;	//长度
		BYTE tableID : 4;	//哈夫曼表ID, 低4位
		HTableType type : 4;	//哈夫曼表适用对象, 高4位, 有DC和AC可选
		BYTE tableEntryLen[16];	//哈夫曼表各项长度,共16项
	};
	BYTE esc;
	BYTE DHTmarker;		//定义符 Marker::DHT
	Info info;					//内容
};

struct JPEG_HTable
{
	JPEG_HTableHeader header;
	BYTE* entries[16];		//哈夫曼表 16项,每项为一个数组
};

struct ImgComponent
{
	Component identifier; //分量ID, YCbCr 3分量情况下, 1对应Y, 2对应Cb, 3对应Cr; 与FrameHeader中的对应
	union {
		struct {
			BYTE AC_HTableID : 4;
			BYTE DC_HTableID : 4;
		};
		BYTE tableSelector;
	};
};

struct JPEG_ScanHeader_BDCT_YCbCr
{
	struct Info
	{
		WORD length;				//长度
		enum : BYTE {
			COMPCNT_GRAY,
			COMPCNT_YUV = 3,
			COMPCNT_CMYK = 4
		}numberOfComponents = COMPCNT_YUV;	//分量数(YCbCr 为 3)
		ImgComponent components[COMPCNT_YUV];
		BYTE startOfSpectralSel = 0;	//谱选择开始,固定为0
		BYTE endOfSpectralSel = 63;	//谱选择结束,固定为63
		BYTE successiveApproxBitPosL : 4 = 0;		//default initialization. C++20 only
		BYTE successiveApproxBitPosH : 4 = 0;		//default initialization. C++20 only
	};
	BYTE esc;
	BYTE SOSmarker;		//定义符Marker::SOS
	Info info;
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
	JPEG_FrameHeader_YCbCr frameHeader;
	JPEG_Scan_4H scanData;
};

//Baseline DCT 2量化表 4哈夫曼表 JPEG 文件结构
struct JPEG_File_BDCT_2Q4H
{
	BYTE esc0;
	BYTE SOImarker; // Marker::SOI;
	JPEG_Frame_BDCT_2Q4H frame;
	BYTE esc1;
	BYTE EOFmarker; // Marker::EOI;
};

struct MCU {
	Block** y;
	Block* cb;
	Block* cr;
};

struct RLCode {
	int zeroCnt;
	int value;
};

#endif
#pragma pack(pop)

#endif // typedef_h__