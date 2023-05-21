#pragma once
/*
* Encoder.h
* Written by kiminouso, 2023/05/01
*/
#ifndef Encoder_h__
#define Encoder_h__
#include <vector>
#include "typedef.h"
#include "BitString.h"
#include "CodingUtil.h"
#include "BitWriter.h"

class Encoder {
private:
	class SymbolTable {
	private:
		size_t* _freqMap;
		BitString* _codeMap;
		int _count;
	public:
		SymbolTable(int entryCount);
		const int count() const;
		size_t& frequency(const int symbol) const;
		BitString& code(const int symbol) const;
		size_t* frequencyMapPtr() const;
		~SymbolTable();
	};

	struct BitCode {
		union {
			struct {
				BYTE bitLength : 4;
				BYTE zeroCnt : 4;
			};
			BYTE codedUnit;
		};
		DWORD bits;
	};

	using BitCodeArray = std::vector<BitCode>;

	struct BitCodeUnit {
		std::vector<BitCodeArray> y;
		BitCodeArray cb;
		BitCodeArray cr;
	};


	SubsampFact _subsampFact{};

	/*MCU的结构:
	* 在图像中的结构:
	* 由水平采样因子h 和 垂直采样因子v决定
	* h/v=1/1
	* +-------+-------+-------+
	* |  MCU1 |  MCU2 |  ...  |
	* |  8x8  |  8x8  |       |
	* +-------+-------+-------+
	* Single MCU:
	* +-------+ +-------+ +-------+
	* | Block | | Block | | Block |
	* |  Y1   | |  Cb   | |  Cr   |
	* +-------+ +-------+ +-------+
	* h/v=1/2
	* +-------+-------+-------+
	* |  MCU1 |  MCU2 |  ...  |
	* |  8x16 |  8x16 |       |
	* +-------+-------+-------+
	* Single MCU:
	* +-------+   +-------+ +-------+
	* | Block |   | Block | | Block |
	* |  Y1   |   |  Cb   | |  Cr   |
	* +-------+   +-------+ +-------+
	* | Block |
	* |  Y2   |
	* +-------+
	* h/v=2/2
	* +-------+-------+-------+
	* |  MCU1 |  MCU2 |  ...  |
	* | 16x16 | 16x16 |       |
	* +-------+-------+-------+
	* Single MCU:
	* +-------+-------+   +-------+ +-------+
	* | Block | Block |   | Block | | Block |
	* |  Y1   |  Y2   |   |  Cb   | |  Cr   |
	* +-------+-------+   +-------+ +-------+
	* | Block | Block |
	* |  Y3   |  Y4   |
	* +-------+-------+
	* 一维存储结构:
	* +-------+-------+-------+-------+-------+-------+
	* | Block | Block |  ...  | Block | Block | Block |
	* |  Y1   |   Y2  |       |   Yn  |  Cb   |   Cr  |
	* +-------+-------+-------+-------+-------+-------+
	*/
	Matrix<MCU>* _MCUs{};
	DPCM _dpcmEncoders[3];
	HuffmanTable  _huffTable_Y_DC;
	HuffmanTable  _huffTable_Y_AC;
	HuffmanTable  _huffTable_CbCr_DC;
	HuffmanTable  _huffTable_CbCr_AC;
	WORD _width{};
	WORD _height{};

	std::vector<BYTE> _codedBytes;

	void _makeBoundaryMCU(const Matrix<YCbCr>& ycbcrData, MCU& mcu, int pos_x, int pos_y);

	//分配一块MCU
	void _allocMCU(MCU* pMCU);

	void _deallocMCU(MCU* pMCU);

	void _makeOneMCU(const Matrix<YCbCr>& ycbcrData, MCU& mcu, int pos_x, int pos_y);

	//注意:更新MCU矩阵时采用浅拷贝
	void _updateMCU(MCU* pOldMCU,MCU* pNewMCU);

	//对每个值减去128以便进行FDCT操作
	void _lshift128(Block* block) {
		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				(*block)[r][c] -= 128;
			}
		}
	}

	//差分编码&游程编码
	//返回一个包装好的BitEncode结果数组BitCodeArray
	void _bitEncodeOneBlock(const Block* input, BitCodeArray& output,DPCM& dpcmEncoder);

	//计算一个BitCodeArray所产生的CodedUnit符号的频数,一个BitCodeArray与一个block对应
	void _accumulateFrequency(const BitCodeArray& input, SymbolTable& tbl_DC,SymbolTable& tbl_AC);

	//计算一个MCU所产生的CodedUnit符号的频数,MCU所产生的BitCodeArray被封装在了BitCodeUnit中
	void _countOneUnitFreq(const BitCodeUnit& bitCodeUnit, SymbolTable& tbl_Y_DC, SymbolTable& tbl_Y_AC, SymbolTable& tbl_CbCr_DC, SymbolTable& tbl_CbCr_AC);

	//计算所有MCU产生的CodedUnit符号的频数,以供生成huffman树
	void _countFrequency(
		const Matrix<BitCodeUnit>& bitCodeMatrix,
		SymbolTable& tbl_Y_DC,
		SymbolTable& tbl_Y_AC, 
		SymbolTable& tbl_CbCr_DC, 
		SymbolTable& tbl_CbCr_AC);

	//先Y,再Cb,再Cr
	//如果采样因子非1:1,则Y的块有多个,在一个MCU中从左往右从上到下遍历处理
	void _bitEncodeOneMCU(MCU* mcu, BitCodeUnit& codeUnit);

	//对每个MCU进行差分编码及游程编码,并得到合并字节后的结果
	void _bitEncode(Matrix<BitCodeUnit>& bitCodeMatrix);

	//获取符号对应的哈夫曼编码,写入SymbolTable,并生成一个HuffmanTable
	void _generateHuffmanTable(SymbolTable& symbolTable,HuffmanTable& huffTable);


	void _huffmanEncodeOneBlock(const BitCodeArray& bitCodes, const SymbolTable& tbl_DC, const SymbolTable& tbl_AC,BitWriter& writer);

	void _huffmanEncodeOneUnit(
		const BitCodeUnit& bitCodeUnit,
		const SymbolTable& tbl_Y_DC,
		const SymbolTable& tbl_Y_AC,
		const SymbolTable& tbl_CbCr_DC,
		const SymbolTable& tbl_CbCr_AC,
		BitWriter& writer);

	void _reserveLastSymbol(SymbolTable& table,const DWORD lastSymbol);

	void _removeReservedSymbol(HuffmanTable& table);

	void _printHuffmanTable(SymbolTable& symbolTable,HuffmanTable& table,const char* description);

	void _huffmanEncode(Matrix<BitCodeUnit>& bitCodeMatrix, std::vector<BYTE>& output);

public:

	void makeMCUs(Matrix<YCbCr>& ycbcrData, SubsampFact subsampFact);

	void doFDCT();

	void doQuantize(float qualityFactor);

	//过程:
	//DC分量差分编码 -->-+
	//                 	+-->对编码结果合并字节操作-->Huffman编码及序列化
	//AC分量游程编码 -->-+
	//+---------------BitEncode-----------------++---HuffmanEncode---+
	void encode();

	void makeJPGFile(const char* fileName);
};
#endif // Encoder_h__