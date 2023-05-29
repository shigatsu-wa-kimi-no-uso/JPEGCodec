/*
* Decoder.h
* Written by kiminouso, 2023/05/26
*/
#pragma once
#ifndef Decoder_h__
#define Decoder_h__
#include "typedef.h"
#include "BitString.h"
#include "BitReader.h"
#include "CodingUtil.h"

class Decoder
{
private:
	struct Symbol {
		BYTE symbol;
		int length;
		size_t freq;
	};

	class SymbolTable 
	{
	private:
		//哈夫曼编码-符号映射表
		//符号不存在时,length==0
		Symbol* _symbols;
		size_t _count;
	public:
		SymbolTable();
		SymbolTable(SymbolTable&& val) noexcept;
		SymbolTable(const SymbolTable& val) noexcept;
		SymbolTable(size_t symbolCnt);
		~SymbolTable();
		void operator=(SymbolTable&& rgt) noexcept;
		void operator=(const SymbolTable& rgt) noexcept;
		bool hasSymbol(const BitString& bitString) const;
		Symbol& symbol(const BitString& bitString);
		const Symbol& symbol(const BitString& bitString) const;
	};
	
	
	//符号表(二维数组)
	//第一维:符号表适应类型(DC/AC)
	//第二维:符号表编号(与JPEG中的哈夫曼表编号相同)
	std::vector<SymbolTable> _symbolTables[(int)HTableType::MAXENUMVAL];
#ifdef STAT_SYMBOL_FREQ
	std::vector<std::vector<std::vector<BitString>>> _bitStringTables[(int)HTableType::MAXENUMVAL];
#endif
	std::vector<const BYTE(*)[BLOCK_ROWCNT][BLOCK_COLCNT]> _quantTables;	//注意:JPG文件中的量化表是以Z字形顺序排列后的量化表(原封不动添加到_quantTables数组,故是Z字形排列的)
	ComponentConfig _cmptCfgs[(int)Component::MAXENUMVAL];
	Matrix<MCU>* _MCUs{};
	DPCM _dpcmDecoders[3];
	WORD _width{};
	WORD _height{};
	SubsampFact _subsampFact{};
	std::vector<BYTE> _codedData;

	//分配一块MCU
	void _allocMCU(MCU& mcu);
	void _deallocMCU(MCU& mcu);
	//注意:更新MCU矩阵时采用浅拷贝
	void _updateMCU(MCU& oldMCU, MCU& newMCU);
	void _initAllocMCUs();
	void _deallocMCUs();
	void _matchSymbol(SymbolTable& table, BitReader& reader, Symbol& symbol);
	void _huffmanDecodeOneBlock(const int tableID_DC, const int tableID_AC, BitCodeArray& bitCodes, BitReader& reader);
	void _huffmanDecodeOneMCU(BitCodeUnit& bitCodeUnit, BitReader& reader);
	[[deprecated]]
	void _huffmanDecode(Matrix<BitCodeUnit>& bitCodeMatrix);
#ifdef STAT_SYMBOL_FREQ
	void _printSymbolTableStatistics();
#endif
	void _bitDecodeOneBlock(const BitCodeArray& bitCodes, Block& block,DPCM& dpcmDecoder,RLCode (&codes)[256]);
	void _bitDecodeOneMCU(const BitCodeUnit& bitCodeUnit, MCU& mcu);
	[[deprecated]]
	void _bitDecode(const Matrix<BitCodeUnit>& bitCodeMatrix);
	void _rshift128(Block& block);
	void _makeYCbCrDataOfOneMCU(const MCU& mcu, int pos_x, int pos_y, Matrix<YCbCr>& ycbcrData);
	void _makeYCbCrDataOfBoundaryMCU(const MCU& mcu, int pos_x, int pos_y, Matrix<YCbCr>& ycbcrData);
	void _normalize(Block& block);
	/*过程:
	*
	* 原始数据-->哈夫曼解码-->RLE解码-->还原每个Block(Z字形排列的)
	* +----HuffmanDecode----++-----------BitDecode-----------+
	*/
	void _decodeOneMCU(MCU& mcu, BitReader& reader);
public:
	Decoder();
	~Decoder();
	void setCodedData(std::vector<BYTE>&& codedData);
	void setQuantTable(const int id,const BYTE (&quantTable)[BLOCK_ROWCNT][BLOCK_COLCNT]);
	void setComponentConfigs(const ComponentConfig(&cmptCfgs)[(int)Component::MAXENUMVAL]);
	void setSize(WORD width,WORD height);
	void setSymbolTable(const int id,const HuffmanTable& huffTable,const HTableType type);
	void decode();
	//对Z字形排列的序列dequantize, 由于量化表也是Z字形排列的,因此在dequantize之后再将其正过来
	void dequantize();
	void doIDCT();
	void makeYCbCrData(Matrix<YCbCr>& ycbcrData);
};

#endif // Decoder_h__
