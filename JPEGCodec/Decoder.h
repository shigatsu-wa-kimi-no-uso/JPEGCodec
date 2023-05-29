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
		//����������-����ӳ���
		//���Ų�����ʱ,length==0
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
	
	
	//���ű�(��ά����)
	//��һά:���ű���Ӧ����(DC/AC)
	//�ڶ�ά:���ű���(��JPEG�еĹ�����������ͬ)
	std::vector<SymbolTable> _symbolTables[(int)HTableType::MAXENUMVAL];
#ifdef STAT_SYMBOL_FREQ
	std::vector<std::vector<std::vector<BitString>>> _bitStringTables[(int)HTableType::MAXENUMVAL];
#endif
	std::vector<const BYTE(*)[BLOCK_ROWCNT][BLOCK_COLCNT]> _quantTables;	//ע��:JPG�ļ��е�����������Z����˳�����к��������(ԭ�ⲻ����ӵ�_quantTables����,����Z�������е�)
	ComponentConfig _cmptCfgs[(int)Component::MAXENUMVAL];
	Matrix<MCU>* _MCUs{};
	DPCM _dpcmDecoders[3];
	WORD _width{};
	WORD _height{};
	SubsampFact _subsampFact{};
	std::vector<BYTE> _codedData;

	//����һ��MCU
	void _allocMCU(MCU& mcu);
	void _deallocMCU(MCU& mcu);
	//ע��:����MCU����ʱ����ǳ����
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
	/*����:
	*
	* ԭʼ����-->����������-->RLE����-->��ԭÿ��Block(Z�������е�)
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
	//��Z�������е�����dequantize, ����������Ҳ��Z�������е�,�����dequantize֮���ٽ���������
	void dequantize();
	void doIDCT();
	void makeYCbCrData(Matrix<YCbCr>& ycbcrData);
};

#endif // Decoder_h__
