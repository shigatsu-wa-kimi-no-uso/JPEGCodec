/*
* JPEGFileBuilder.h
* Written by kiminouso, 2023/05/20
*/

#pragma once
#ifndef JPEGFileBuilder_h__
#define JPEGFileBuilder_h__
#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include "typedef.h"


class JPEGFileBuilder
{
private:
	std::vector<std::pair<const BYTE, const BYTE*>> _quantTables;
	std::vector<std::tuple<const BYTE, const HuffmanTable, const HTableType>> _huffTables;
	BYTE _cmptAdoptedHTable[(int)Component::MAXENUMVAL][(int)HTableType::MAXENUMVAL]{};
	FILE* _hFile{};
	const BYTE* _codedBytes{};
	size_t _codedBytesLen{};
	WORD _width{};
	WORD _height{};
	SubsampFact _subsampFact{};

	void _writeJFIFHeader();

	void _writeQuantTable(const BYTE id, const BYTE* quantTable);

	void _writeFrameHeader();

	void _writeHuffmanTable(const BYTE id,const HuffmanTable& table,const HTableType type);

	void _writeScanHeader();

	void _writeCodedBytes();

public:
	JPEGFileBuilder();

	JPEGFileBuilder& setQuantTable(const BYTE id,const BYTE* quantTable);

	JPEGFileBuilder& setHuffmanTable(const BYTE id,const HuffmanTable huffTable,const HTableType type);

	JPEGFileBuilder& setCmptAdoptedHTable(const Component appliedCmpt, const HTableType type,  const BYTE id);

	JPEGFileBuilder& setCodedBytes(const std::vector<BYTE>& codedBytes);

	JPEGFileBuilder& setHeight(const WORD height);

	JPEGFileBuilder& setWidth(const WORD width);

	JPEGFileBuilder& setSubsampFact(const SubsampFact& subsampFact);

	void makeJPGFile(const char* fileName);
};
#endif // JPEGFileBuilder_h__
