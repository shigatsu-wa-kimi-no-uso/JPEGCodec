
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
		bool open(const char* fileName) {
			_hFile = fopen(fileName, "rb");
			return _hFile != nullptr;
		}

		BYTE readByte() {
			BYTE buf;
			fread(&buf, sizeof(BYTE), 1, _hFile);
			return buf;
		}

		WORD readWord() {
			WORD buf;
			fread(&buf, sizeof(WORD), 1, _hFile);
			return host_order(buf);
		}

		void read(void* buf, size_t size) {
			fread(buf, sizeof(BYTE), size, _hFile);
		}

		void skip(size_t skipByteCnt) {
			fseek(_hFile, skipByteCnt, SEEK_CUR);
		}

		bool fileEnd() {
			return feof(_hFile);
		}
		void close() {
			fclose(_hFile);
		}
	};

	ComponentConfig _cmptCfgs[(int)Component::MAXENUMVAL];

	std::vector<std::pair<int, BYTE(*)[8][8]>> _quantTables;
	std::vector<std::pair<int, HuffmanTable>> _huffTables[(int)HTableType::MAXENUMVAL];
	WORD _width;
	WORD _height;
	std::vector<BYTE> _codedData;

	void _resolveJFIFHeader(JPEGFileReader& reader) {
		JPEG_JFIFHeader::Info info;
		reader.read(&info, sizeof(info));
		reader.skip(info.XThumbnail * info.YThumbnail * 3);
	}

	void _resolveQuantTable(JPEGFileReader& reader) {
		WORD length = reader.readWord();
		JPEG_QTableHeader::Info info;
		reader.read(&info, sizeof(info));
		if (info.precision != JPEG_QTableHeader::Info::PREC_8BIT) {
			fputs("Unsupported quantization table precision.", stderr);
		}
		if (host_order(info.length) != sizeof(info) + BLOCK_COLCNT*BLOCK_ROWCNT) {
			fputs("Structure around 'DQT' is malformed.",stderr);
		}
		std::pair<int, BYTE(*)[8][8]> entry;
		entry.first = info.tableID;
		reader.read(entry.second, sizeof(entry.second));
		_quantTables.push_back(entry);
	}

	void _resolveFrameHeader(JPEGFileReader& reader) {
		JPEG_FrameHeader_YCbCr::Info info;
		reader.read(&info, sizeof(info));
		if (info.precision != JPEG_FrameHeader_YCbCr::Info::PREC_8BIT) {
			fputs("Unsupported sampling precision.", stderr);
		}
		if (info.numberOfComponents != JPEG_FrameHeader_YCbCr::Info::COMPCNT_YUV) {
			fputs("Unsupported number of components.", stderr);
		}
		_width = host_order(info.samplesPerLine);
		_height = host_order(info.numberOfLines);

		//info.components 一定有3个元素
		for (int i = 0; i < 3; ++i) {
			_cmptCfgs[(int)info.components[i].identifier].QTableSel = info.components[i].qtableID;
			_cmptCfgs[(int)info.components[i].identifier].subsampFact.rawVal = info.components[i].subsampFact;
		}
	}

	void _resolveHuffmanTable(JPEGFileReader& reader) {
		JPEG_HTableHeader::Info info;
		reader.read(&info, sizeof(info));
		HuffmanTable table;
		table.resize(sizeof(info.tableEntryLen));
		for (int i = 0; i < sizeof(info.tableEntryLen); ++i) {
			table[i].resize(info.tableEntryLen[i]);
			for (int j = 0; j < info.tableEntryLen[i]; ++j) {
				table[i][j] = reader.readByte();
			}
		}
		_huffTables[(int)info.type].push_back(std::pair(info.tableID, std::move(table)));
	}

	void _resolveScanHeader(JPEGFileReader& reader) {
		WORD length = reader.readWord();
		JPEG_ScanHeader_BDCT_YCbCr::Info info;
		reader.read(&info, sizeof(info));
		if (info.numberOfComponents != JPEG_FrameHeader_YCbCr::Info::COMPCNT_YUV) {
			fputs("Unsupported number of components.", stderr);
		}
		//info.components 一定有3个元素
		for (int i = 0; i < 3; ++i) {
			_cmptCfgs[(int)info.components[i].identifier].AC_HTableSel = info.components[i].AC_HTableID;
			_cmptCfgs[(int)info.components[i].identifier].DC_HTableSel = info.components[i].DC_HTableID;
		}
	}

	void _readScanData(JPEGFileReader& reader) {
		BYTE curr;
		bool read_ESC = false;
		bool read_EOI = false;
		while (!reader.fileEnd() && !read_EOI) {
			curr = reader.readByte();
			//jpg 编码数据中,0xFF后面应为00
			if (read_ESC) {
				if (curr == Marker::DAT_NIL) {
					_codedData.push_back(curr);
					read_ESC = false;
				} else if(curr != Marker::EOI){
					read_ESC = false;
					read_EOI = true;
				} else {
					fprintf(stderr, "Byte '0xFF' is followed by '%#x'. Expected '0x0' or '0xD9'.\n", curr);
				}
			} else {
				if (curr == Marker::ESC) {
					read_ESC = true;
				} else {
					_codedData.push_back(curr);
				}
			}
		}
		if (!read_EOI) {
			fputs("File ended without EOI marker!\n", stderr);
		}
	}

public:
	bool load(const char* fileName) {
		JPEGFileReader reader;
		reader.open(fileName);
		BYTE last = 0;
		BYTE curr = 0;
		bool read_SOI = false;
		bool read_SOS = false;
		bool read_DQT = false;
		bool read_SOF = false;
		bool read_DHT = false;
		const char* notfounderrstr = "Cannot find %s marker.\n";
		//根据JPEG文件结构读取文件信息
		while (!reader.fileEnd() && read_SOS == false) {
			last = curr;
			curr = reader.readByte();
			if (last == Marker::ESC) {
				if (Marker::APP1 <= curr && curr <= Marker::APP15) {
					continue;
				}
				//根据标志位分别处理并收集解码所需信息
				switch (curr) {
				case Marker::SOI:
					read_SOI = true;
					break;
				case Marker::APP0:
					_resolveJFIFHeader(reader);
					break;
				case Marker::DQT:
					_resolveQuantTable(reader);
					read_DQT = true;
					break;
				case Marker::BASELINE_DCT:
					_resolveFrameHeader(reader);
					read_SOF = true;
					break;
				case Marker::DHT:
					_resolveHuffmanTable(reader);
					read_DHT = true;
					break;
				case Marker::SOS:
					_resolveScanHeader(reader);
					read_SOS = true;
					break;
				default:
					fprintf(stderr,"Unresolved marker:0xFF%x\n",curr);
				}
			}
		}
		if (!read_SOI) {
			fprintf(stderr, notfounderrstr,"SOI");
			return false;
		}
		if (!read_DQT) {
			fprintf(stderr, notfounderrstr, "DQT");
			return false;
		}
		if (!read_SOF) {
			fprintf(stderr, notfounderrstr, "SOF");
			return false;
		}
		if (!read_DHT) {
			fprintf(stderr, notfounderrstr, "DHT");
			return false;
		}
		if (!read_SOS) {
			fprintf(stderr, notfounderrstr, "SOS");
			return false;
		}
		_readScanData(reader);
		return true;
	}

	const ComponentConfig (&getComponentConfigs() const )[(int)Component::MAXENUMVAL] {
		return _cmptCfgs;
	}

	WORD getWidth() const {
		return _width;
	}

	WORD getHeight() const {
		return _height;
	}

	std::vector<BYTE>& getCodedData() {
		return _codedData;
	}

	const std::vector<std::pair<int, BYTE(*)[8][8]>> getQuantTables() const {
		return _quantTables;
	}

	const std::vector<std::pair<int, HuffmanTable>> (&getHuffmanTables() const)[(int)HTableType::MAXENUMVAL] {
		return _huffTables;
	}
};

#endif // JPEGFileLoader_h__
