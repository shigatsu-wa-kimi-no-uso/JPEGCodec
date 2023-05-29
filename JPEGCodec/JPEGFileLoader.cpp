/*
* JPEGFileLoader.h
* Written by kiminouso, 2023/05/23
*/
#include "JPEGFileLoader.h"

bool JPEGFileLoader::JPEGFileReader::open(const char* fileName) {
	_hFile = fopen(fileName, "rb");
	return _hFile != nullptr;
}

BYTE JPEGFileLoader::JPEGFileReader::readByte() {
	BYTE buf;
	fread(&buf, sizeof(BYTE), 1, _hFile);
	return buf;
}

WORD JPEGFileLoader::JPEGFileReader::readWord() {
	WORD buf;
	fread(&buf, sizeof(WORD), 1, _hFile);
	return host_order(buf);
}

size_t JPEGFileLoader::JPEGFileReader::position() {
	return ftell(_hFile);
}

void JPEGFileLoader::JPEGFileReader::read(void* buf, size_t size) {
	fread(buf, sizeof(BYTE), size, _hFile);
}

void JPEGFileLoader::JPEGFileReader::skip(long skipByteCnt) {
	fseek(_hFile, skipByteCnt, SEEK_CUR);
}

bool JPEGFileLoader::JPEGFileReader::fileEnd() {
	return feof(_hFile);
}

void JPEGFileLoader::JPEGFileReader::close() {
	fclose(_hFile);
}

void JPEGFileLoader::_resolveJFIFHeader(JPEGFileReader& reader) {
	JPEG_JFIFHeader::Info info;
	reader.read(&info, sizeof(info));
	reader.skip(info.XThumbnail * info.YThumbnail * 3);
}

void JPEGFileLoader::_resolveQuantTable(JPEGFileReader& reader) {
	JPEG_QTableHeader::Info info;
	reader.read(&info, sizeof(info));
	if (info.precision != JPEG_QTableHeader::Info::PREC_8BIT) {
		fputs("Unsupported quantization table precision.", stderr);
	}
	if (host_order(info.length) != sizeof(info) + BLOCK_COLCNT * BLOCK_ROWCNT) {
		fputs("Structure around 'DQT' is malformed.", stderr);
	}
	_quantTables.emplace_back();
	_quantTables.back().first = info.tableID;
	reader.read(_quantTables.back().second, sizeof(_quantTables.back().second));
}

void JPEGFileLoader::_resolveFrameHeader(JPEGFileReader& reader) {
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

void JPEGFileLoader::_resolveHuffmanTable(JPEGFileReader& reader) {
	JPEG_HTableHeader::Info info;
	reader.read(&info, sizeof(info));
	_huffTables[(int)info.type].emplace_back();
	HuffmanTable& table = _huffTables[(int)info.type].back().second;
	table.resize(sizeof(info.tableEntryLen) + 1);
	for (int i = 0; i < sizeof(info.tableEntryLen); ++i) {
		table[i + 1].resize(info.tableEntryLen[i]);
		for (int j = 0; j < info.tableEntryLen[i]; ++j) {
			table[i + 1][j] = reader.readByte();
		}
	}
	_huffTables[(int)info.type].back().first = info.tableID;
}

void JPEGFileLoader::_resolveScanHeader(JPEGFileReader& reader) {
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

void JPEGFileLoader::_readScanData(JPEGFileReader& reader) {
	BYTE curr;
	bool read_ESC = false;
	bool read_EOI = false;
	while (!reader.fileEnd() && !read_EOI) {
		curr = reader.readByte();
		//jpg 编码时 0xFF -> 0xFF00 , 解码时 0xFF00 -> 0xFF
		if (read_ESC) {
			if (curr == Marker::DAT_NIL) {
				_codedData.push_back(0xFF);
				read_ESC = false;
			} else if (curr == Marker::EOI) {
				read_ESC = false;
				read_EOI = true;
			} else {
				fprintf(stderr, "Byte '0xff' is followed by '%#x'. Expected '0x0' or '0xd9'.\n", curr);
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

bool JPEGFileLoader::load(const char* fileName) {
	JPEGFileReader reader;
	if (!reader.open(fileName)) {
		return false;
	}
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
				fprintf(stderr, "Unresolved marker '0xff%02x' at %#zx\n", curr, reader.position() - 2);
			}
		}
	}
	if (!read_SOI) {
		fprintf(stderr, notfounderrstr, "SOI");
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
	reader.close();
	return true;
}

const ComponentConfig(&JPEGFileLoader::getComponentConfigs() const)[(int)Component::MAXENUMVAL] {
	return _cmptCfgs;
}

WORD JPEGFileLoader::getWidth() const {
	return _width;
}

WORD JPEGFileLoader::getHeight() const {
	return _height;
}

std::vector<BYTE>& JPEGFileLoader::getCodedData() {
	return _codedData;
}

const std::vector<std::pair<int, BYTE[8][8]>>& JPEGFileLoader::getQuantTables() const {
	return _quantTables;
}

const std::vector<std::pair<int, HuffmanTable>>(&JPEGFileLoader::getHuffmanTables() const)[(int)HTableType::MAXENUMVAL] {
	return _huffTables;
}
