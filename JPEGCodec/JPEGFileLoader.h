/*
* JPEGFileLoader.h
* Written by kiminouso, 2023/05/23
*/
#pragma once
#ifndef JPEGFileLoader_h__
#define JPEGFileLoader_h__
#include "typedef.h"
#include "UtilFunc.h"

class JPEGFileLoader 
{
private:
	class JPEGFileReader 
	{
	private:
		FILE* _hFile;
	public:
		bool open(const char* fileName);
		BYTE readByte();
		WORD readWord();
		size_t position();
		void read(void* buf, size_t size);
		void skip(long skipByteCnt);
		bool fileEnd();
		void close();
	};

	ComponentConfig _cmptCfgs[(int)Component::MAXENUMVAL];
	std::vector<std::pair<int, BYTE[8][8]>> _quantTables;
	std::vector<std::pair<int, HuffmanTable>> _huffTables[(int)HTableType::MAXENUMVAL];
	WORD _width;
	WORD _height;
	std::vector<BYTE> _codedData;

	void _resolveJFIFHeader(JPEGFileReader& reader);
	void _resolveQuantTable(JPEGFileReader& reader);
	void _resolveFrameHeader(JPEGFileReader& reader);
	void _resolveHuffmanTable(JPEGFileReader& reader);
	void _resolveScanHeader(JPEGFileReader& reader);
	void _readScanData(JPEGFileReader& reader);
public:
	bool load(const char* fileName);
	const ComponentConfig (&getComponentConfigs() const )[(int)Component::MAXENUMVAL];
	WORD getWidth() const;
	WORD getHeight() const;
	std::vector<BYTE>& getCodedData();
	const std::vector<std::pair<int, BYTE[8][8]>>& getQuantTables() const;
	const std::vector<std::pair<int, HuffmanTable>> (&getHuffmanTables() const)[(int)HTableType::MAXENUMVAL];
};

#endif // JPEGFileLoader_h__