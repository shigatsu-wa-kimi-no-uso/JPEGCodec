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

#pragma pack(push,1)	//���ýṹ��Ϊ1�ֽڶ���

union SubsampFact{
	enum : BYTE {
		YUV_444 = 0x11,
		YUV_440 = 0x12,
		YUV_422 = 0x21,
		YUV_420 = 0x22
	}factor;
	BYTE factor_v : 4;	//��4λ ��ֱ��������
	BYTE factor_h : 4;	//��4λ ˮƽ��������
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
		SOI = 0xFFD8,	//�ļ���ʼ��ʶ��
		EOI = 0xFFD9,	//�ļ�������ʶ��
		SOS = 0xFFDA,	//���ݶ�ɨ����ʼ��
		DQT = 0xFFDB,	//���������
		DNL = 0xFFDC,	//number of lines definer
		DRI = 0xFFDD,	//restart interval definer
		DHP = 0xFFDE,	//hierarchical progression definer
		EXP = 0xFFDF,	//expand reference components
		COM = 0xFFFE,	//ע��
		DAT_NIL=0xFF00	//���ݶε��ֽ�Ϊ0
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

	enum SOFMarker //֡��ʼ����, �������/�ǲ��ʽ, �����ù��������뷽ʽ
	{
		BASELINE_DCT = 0xFFC0,
		EXTENDED_SEQ_DCT = 0xFFC1,
		PROG_DCT = 0xFFC2,
		LOSSLESS = 0xFFC3,
		DIFF_SEQ_DCT = 0xFFC5,
		DIFF_PROG_DCT = 0xFFC6,
		DIFF_LOSSLESS = 0xFFC7,
	};

	enum HTSpec	//�������������
	{
		DHT = 0xFFC4
	};
};

//JFIFͷ
struct JPEG_JFIFHeader
{
	struct Info
	{
		WORD length;		//�öγ���(��ȥmarker�ĳ���) length = 16 + 3*Xthumbnail*Ythumbnail
		char identifier[5] = "JFIF";	//��ʶ�� "JFIF"��ASCII��, '\0'��β
		BYTE version[2];	//�汾 1.01(version[0]=1 version[1]=1) ��1.02(version[0]=1 version[1]=2) etc 
		enum Units : BYTE {
			NONE,	//��, ʹ�������ݺ��
			PPI,	//ÿӢ������
			PPCM	//ÿ��������
		} units;	//�ܶȵ�λ		

		WORD Xdensity;		//ˮƽ�ֱ���
		WORD Ydensity;		//��ֱ�ֱ���(unitsΪ0ʱ,Xdensity��Ydensity��ʾ�����ݺ��,ͨ��ȡXdensity=1,Ydensity=1)
		BYTE XThumbnail;	//����ͼ��ˮƽ����
		BYTE YThumbnail;	//����ͼ�Ĵ�ֱ����
		//sizeof(JPEG_JFIFHeader::Info) == 16
	};

	WORD marker = Marker::APP0;		//��ʼ���
	Info info;						//����
};

struct JPEG_QTableHeader
{
	struct Info
	{
		WORD length;		//���� length = sizeof(Info) + ��������
		BYTE tableID : 4;	//��4λ id
		enum PRECISION : BYTE
		{
			PREC_8BIT,
			PREC_16BIT
		}precision : 4;	//��4λ ���� 0Ϊ8bit, 1Ϊ16bit
	};
	const WORD marker = Marker::DQT;	//�����
	Info info;							//����
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
		WORD length;		//ͷ����
		const enum PRECISION : BYTE
		{
			PREC_12BIT,
			PREC_8BIT
		}precision = PREC_8BIT ;		//��������, ����Ϊ8bit
		WORD numberOfLines;	//�߶�����
		WORD samplesPerLine;//�������
		const enum : BYTE {
			COMPCNT_GRAY,
			COMPCNT_YUV = 3,
			COMPCNT_CMYK = 4
		}numberOfComponents = COMPCNT_YUV;	//������(YCbCrΪ3, ֻ�лҶ�Ϊ1, CMYKΪ4)
		struct ImgComponent 
		{
			enum : BYTE 
			{
				LUMA = 0x1,
				CHROMA_B,
				CHROMA_R
			}identifier; //����ID, YCbCr 3���������, 1��ӦY, 2��ӦCb, 3��ӦCr
			union {
				BYTE _subsampFact;
				BYTE VSubsampFact : 4;	//��4λ ��ֱ��������
				BYTE HSubsampFact : 4;	//��4λ ˮƽ��������
			};
			BYTE qtableID;			//������ID
		} components[COMPCNT_YUV];			//YCbCr����3����
	};
	const WORD marker = Marker::BASELINE_DCT;	//DCT���ͱ��
	Info info;									//����
};



struct JPEG_HTableHeader
{
	struct Info
	{
		WORD length;	//����
		BYTE tableID : 4;	//��������ID, ��4λ
		enum : BYTE
		{
			DC,
			AC
		}type : 4;				//�����������ö���, ��4λ, ��DC��AC��ѡ
		BYTE tableEntryLen[16];	//������������,��16��
	};
	const WORD marker = Marker::DHT;		//�����
	Info info;								//����
};

struct JPEG_HTable
{
	JPEG_HTableHeader header;
	BYTE* entries[16];		//�������� 16��,ÿ��Ϊһ������
};

struct JPEG_ScanHeader_BDCT_YCbCr
{
	const WORD marker = Marker::SOS;		//�����
	struct Info
	{
		WORD length;				//����
		const enum : BYTE {
			COMPCNT_GRAY,
			COMPCNT_YUV = 3,
			COMPCNT_CMYK = 4
		}numberOfComponents = COMPCNT_YUV;	//������(YCbCr Ϊ 3)
		struct ImgComponent
		{
			enum : BYTE
			{
				LUMA = 0x1,
				CHROMA_B,
				CHROMA_R
			}identifier; //����ID, YCbCr 3���������, 1��ӦY, 2��ӦCb, 3��ӦCr; ��FrameHeader�еĶ�Ӧ
			BYTE AC_HTableID : 4;
			BYTE DC_HTableID : 4;
		}components[COMPCNT_YUV];
		const BYTE startOfSpectralSel = 0;	//��ѡ��ʼ,�̶�Ϊ0
		const BYTE endOfSpectralSel = 63;	//��ѡ�����,�̶�Ϊ63
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

//Baseline DCT 2������ 4�������� JPEG �ļ��ṹ
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