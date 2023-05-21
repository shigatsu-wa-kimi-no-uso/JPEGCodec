#include "JPEGFileBuilder.h"
#include "UtilFunc.h"

void JPEGFileBuilder::_writeJFIFHeader() {
	//JFIF Header
	JPEG_JFIFHeader jfifHeader = {
		big_endian(Marker::APP0),
		{
			big_endian((WORD)sizeof(JPEG_JFIFHeader::Info)),
			"JFIF",
			{ 1,1 },
			JPEG_JFIFHeader::Info::Units::NONE,
			big_endian((WORD)1),
			big_endian((WORD)1),
			0,
			0
		}
	};
	fwrite(&jfifHeader, sizeof(jfifHeader), 1, _hFile);
}

void JPEGFileBuilder::_writeQuantTable(const BYTE id, const BYTE* quantTable) {
	WORD length = sizeof(JPEG_QTableHeader::Info) + BLOCK_COLCNT * BLOCK_ROWCNT;
	JPEG_QTableHeader qtHeader1 = {
		big_endian(Marker::DQT),
		{
			big_endian(length),
			id,
			JPEG_QTableHeader::Info::PREC_8BIT
		}
	};
	BYTE zigzaggedQuantTable[64];
	getZigzaggedSequence(quantTable, zigzaggedQuantTable);
	fwrite(&qtHeader1, sizeof(qtHeader1), 1, _hFile);
	fwrite(zigzaggedQuantTable, sizeof(zigzaggedQuantTable), 1, _hFile);
}

void JPEGFileBuilder::_writeFrameHeader() {
	JPEG_FrameHeader_YCbCr header = {
		big_endian(Marker::BASELINE_DCT),
		{
			big_endian((WORD)sizeof(JPEG_FrameHeader_YCbCr::Info)),
			JPEG_FrameHeader_YCbCr::Info::PREC_8BIT,
			big_endian(_height),
			big_endian(_width),
			JPEG_FrameHeader_YCbCr::Info::COMPCNT_YUV,
			{
				{
					JPEG_FrameHeader_YCbCr::Info::ImgComponent::LUMA,
					_subsampFact.factor,
					0
				},
				{
						JPEG_FrameHeader_YCbCr::Info::ImgComponent::CHROMA_B,
						0x11,
						1
				},
				{
						JPEG_FrameHeader_YCbCr::Info::ImgComponent::CHROMA_R,
						0x11,
						1
				}
			}
		}
	};
	fwrite(&header, sizeof(header), 1, _hFile);
}

void JPEGFileBuilder::_writeHuffmanTable(const BYTE id, const HuffmanTable& table, const HTableType type) {
	std::vector<BYTE> tableData;
	BYTE tableEntryLen[16] = { 0 };
	size_t len = min(table.size(), 17);
	for (size_t i = 1; i < len; ++i) {
		tableEntryLen[i - 1] = (BYTE)table[i].size();
		for (const BYTE symbol : table[i]) {
			tableData.push_back(symbol);
		}
	}
	JPEG_HTableHeader header = {
		big_endian(Marker::DHT),
		{
			big_endian((WORD)(sizeof(JPEG_HTableHeader::Info) + tableData.size())),
			id,
			type,
			{ 0 }
		}
	};
	memcpy(header.info.tableEntryLen, tableEntryLen, sizeof(tableEntryLen));
	fwrite(&header, sizeof(header), 1, _hFile);
	fwrite(tableData.data(), sizeof(BYTE), tableData.size(), _hFile);
}

void JPEGFileBuilder::_writeScanHeader() {
	JPEG_ScanHeader_BDCT_YCbCr header = {
		big_endian(Marker::SOS),
		{
			big_endian((WORD)sizeof(JPEG_ScanHeader_BDCT_YCbCr::Info)),
			JPEG_ScanHeader_BDCT_YCbCr::Info::COMPCNT_YUV,
			{
				{
					Component::LUMA,
					_cmptAdoptedHTable[(int)Component::LUMA][(int)HTableType::AC],
					_cmptAdoptedHTable[(int)Component::LUMA][(int)HTableType::DC]
				},
				{
					Component::CHROMA_B,
					_cmptAdoptedHTable[(int)Component::CHROMA_B][(int)HTableType::AC],
					_cmptAdoptedHTable[(int)Component::CHROMA_B][(int)HTableType::DC]
				},
				{
					Component::CHROMA_R,
					_cmptAdoptedHTable[(int)Component::CHROMA_R][(int)HTableType::AC],
					_cmptAdoptedHTable[(int)Component::CHROMA_R][(int)HTableType::DC]
				},
			},
			0,
			63,
			0,
			0
		}
	};
	fwrite(&header, sizeof(header), 1, _hFile);
}

void JPEGFileBuilder::_writeCodedBytes() {
	fwrite(_codedBytes, sizeof(BYTE), _codedBytesLen, _hFile);
}

JPEGFileBuilder::JPEGFileBuilder() {}

JPEGFileBuilder& JPEGFileBuilder::setQuantTable(const BYTE id, const BYTE* quantTable) {
	_quantTables.push_back({ id, quantTable });
	return *this;
}

JPEGFileBuilder& JPEGFileBuilder::setHuffmanTable(const BYTE id, const HuffmanTable huffTable, const HTableType type) {
	_huffTables.push_back({ id,huffTable,type });
	return *this;
}

JPEGFileBuilder& JPEGFileBuilder::setCmptAdoptedHTable(const Component appliedCmpt, const HTableType type, const BYTE id) {
	_cmptAdoptedHTable[(int)appliedCmpt][(int)type] = id;
	return *this;
}

JPEGFileBuilder& JPEGFileBuilder::setCodedBytes(const std::vector<BYTE>& codedBytes) {
	_codedBytes = codedBytes.data();
	_codedBytesLen = codedBytes.size();
	return *this;
}

JPEGFileBuilder& JPEGFileBuilder::setHeight(const WORD height) {
	_height = height;
	return *this;
}

JPEGFileBuilder& JPEGFileBuilder::setWidth(const WORD width) {
	_width = width;
	return *this;
}

JPEGFileBuilder& JPEGFileBuilder::setSubsampFact(const SubsampFact& subsampFact) {
	_subsampFact = subsampFact;
	return *this;
}

void JPEGFileBuilder::makeJPGFile(const char* fileName) {
	_hFile = fopen(fileName, "wb");
	//SOI
	WORD SOImarker = big_endian(Marker::SOI);
	fwrite(&SOImarker, sizeof(SOImarker), 1, _hFile);
	_writeJFIFHeader();
	for (const std::pair<const BYTE, const BYTE*>& entry : _quantTables) {
		_writeQuantTable(entry.first, entry.second);
	}
	_writeFrameHeader();
	for (const std::tuple<BYTE, HuffmanTable, HTableType>& entry : _huffTables) {
		_writeHuffmanTable(std::get<0>(entry), std::get<1>(entry), std::get<2>(entry));
	}
	_writeScanHeader();
	_writeCodedBytes();
	//EOI
	WORD EOFMarker = big_endian(Marker::EOI);
	fwrite(&EOFMarker, sizeof(EOFMarker), 1, _hFile);
	fclose(_hFile);
}
