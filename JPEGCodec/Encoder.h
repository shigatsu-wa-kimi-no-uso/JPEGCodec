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

	/*MCU�Ľṹ:
	* ��ͼ���еĽṹ:
	* ��ˮƽ��������h �� ��ֱ��������v����
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
	* һά�洢�ṹ:
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

	//����һ��MCU
	void _allocMCU(MCU* pMCU);

	void _deallocMCU(MCU* pMCU);

	void _makeOneMCU(const Matrix<YCbCr>& ycbcrData, MCU& mcu, int pos_x, int pos_y);

	//ע��:����MCU����ʱ����ǳ����
	void _updateMCU(MCU* pOldMCU,MCU* pNewMCU);

	//��ÿ��ֵ��ȥ128�Ա����FDCT����
	void _lshift128(Block* block) {
		for (int r = 0; r < BLOCK_ROWCNT; ++r) {
			for (int c = 0; c < BLOCK_COLCNT; ++c) {
				(*block)[r][c] -= 128;
			}
		}
	}

	//��ֱ���&�γ̱���
	//����һ����װ�õ�BitEncode�������BitCodeArray
	void _bitEncodeOneBlock(const Block* input, BitCodeArray& output,DPCM& dpcmEncoder);

	//����һ��BitCodeArray��������CodedUnit���ŵ�Ƶ��,һ��BitCodeArray��һ��block��Ӧ
	void _accumulateFrequency(const BitCodeArray& input, SymbolTable& tbl_DC,SymbolTable& tbl_AC);

	//����һ��MCU��������CodedUnit���ŵ�Ƶ��,MCU��������BitCodeArray����װ����BitCodeUnit��
	void _countOneUnitFreq(const BitCodeUnit& bitCodeUnit, SymbolTable& tbl_Y_DC, SymbolTable& tbl_Y_AC, SymbolTable& tbl_CbCr_DC, SymbolTable& tbl_CbCr_AC);

	//��������MCU������CodedUnit���ŵ�Ƶ��,�Թ�����huffman��
	void _countFrequency(
		const Matrix<BitCodeUnit>& bitCodeMatrix,
		SymbolTable& tbl_Y_DC,
		SymbolTable& tbl_Y_AC, 
		SymbolTable& tbl_CbCr_DC, 
		SymbolTable& tbl_CbCr_AC);

	//��Y,��Cb,��Cr
	//����������ӷ�1:1,��Y�Ŀ��ж��,��һ��MCU�д������Ҵ��ϵ��±�������
	void _bitEncodeOneMCU(MCU* mcu, BitCodeUnit& codeUnit);

	//��ÿ��MCU���в�ֱ��뼰�γ̱���,���õ��ϲ��ֽں�Ľ��
	void _bitEncode(Matrix<BitCodeUnit>& bitCodeMatrix);

	//��ȡ���Ŷ�Ӧ�Ĺ���������,д��SymbolTable,������һ��HuffmanTable
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

	//����:
	//DC������ֱ��� -->-+
	//                 	+-->�Ա������ϲ��ֽڲ���-->Huffman���뼰���л�
	//AC�����γ̱��� -->-+
	//+---------------BitEncode-----------------++---HuffmanEncode---+
	void encode();

	void makeJPGFile(const char* fileName);
};
#endif // Encoder_h__