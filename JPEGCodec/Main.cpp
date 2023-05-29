#include<stdio.h>
#include "ColorSpaceConverter.h"
#include "Encoder.h"
#include "Decoder.h"
#include "UtilFunc.h"
#include "BMPFileLoader.h"
#include "BMPFileBuilder.h"
#include "JPEGFileLoader.h"

static const char* src;
static const char* dst;
static SubsampFact subsampFact;
static double qualityFactor;
static const char* opt_fact_h = "-fact_h";
static const char* opt_fact_v = "-fact_v";
static const char* opt_src = "-src";
static const char* opt_dst = "-dst";
static const char* opt_quality = "-quality";
static const char* opt_help = "-h";
static const char* opt_encode = "-encode";
static const char* opt_decode = "-decode";

static enum class WORKING_MODE {
    UNDEFINED,
    ENCODE,
    DECODE
}mode;

void printHelp() {
    char buf[256];
    puts("Usage:");
	sprintf(buf, "%s", opt_encode);
	printf("%24s %s\n", buf, "Program mode specifier. Let the program work as an encoder and encode the source file to JPEG format.");
	sprintf(buf, "%s", opt_decode);
	printf("%24s %s\n", buf, "Program mode specifier. Let the program work as a decoder and decode the source file to BMP format.");
    sprintf(buf, "%s [arg]", opt_fact_h);
    printf("%24s %s\n", buf, "Specifies horizontal subsampling factor by an integer [arg]. 1<= [arg] <= 2");
    sprintf(buf, "%s [arg]", opt_fact_v);
    printf("%24s %s\n", buf, "Specifies vertical subsampling factor by an integer [arg]. 1<= [arg] <= 2");
    sprintf(buf, "%s [arg]", opt_src);
    printf("%24s %s\n", buf, "Specifies a source file (.bmp/.jpg).");
    sprintf(buf, "%s [arg]", opt_dst);
    printf("%24s %s\n", buf, "Creates the JPG/BMP file with the specified file path.");
    sprintf(buf, "%s [arg]", opt_quality);
    printf("%24s %s\n", buf, "Specifies a quality factor. 0 < [arg] <= 100");
    printf("%24s %s\n", opt_help, "Shows this info.");
}

bool initialize(int argc, char** argv) {
    mode = WORKING_MODE::UNDEFINED;
    if (testopt(argc, argv, opt_help)) {
        printHelp();
        return true;
    }

    if (testopt(argc, argv, opt_encode)) {
        mode = WORKING_MODE::ENCODE;
        const char* fact_h = getoptarg(argc, argv, opt_fact_h);
        const char* fact_v = getoptarg(argc, argv, opt_fact_v);
        const char* szQualityFactor = getoptarg(argc, argv, opt_quality);
        if(fact_h == nullptr || fact_v == nullptr || szQualityFactor == nullptr) {
            return false;
        }
        subsampFact.factor_h = atoi(fact_h);
        subsampFact.factor_v = atoi(fact_v);
        qualityFactor = atof(szQualityFactor);
        if ((int)subsampFact.factor_h < 0 
            || (int)subsampFact.factor_v < 0 
            || (int)subsampFact.factor_h > 2 
            || (int)subsampFact.factor_v > 2) {
            fputs("Subsampling factor should be either 1 or 2.", stderr);
            return false;
        }

		if (qualityFactor <= 0 || qualityFactor > 100) {
			fputs("Quality factor should not smaller than 1 or bigger than 100.", stderr);
            return false;
		}
    } else if (testopt(argc, argv, opt_decode)) {
        mode = WORKING_MODE::DECODE;
    } else {
        return false;
    }

    src = getoptarg(argc, argv, opt_src);
    dst = getoptarg(argc, argv, opt_dst);
    if (src == nullptr || dst == nullptr) {
        return false;
    }

    return true;
}


void _writeYCbCr(const Matrix<YCbCr>& ycbcrData, const char* filePath) {
    BYTE extraByte[3] = { 0 };
    FILE* hFile = fopen(filePath, "wb");
    for (int r = ycbcrData.row_cnt - 1; r >= 0; --r) {
        fwrite(ycbcrData.row_pointer(r), sizeof(RGBTriple), ycbcrData.row_cnt, hFile);
    }
    fclose(hFile);
}

void encode(const char* srcFilePath,SubsampFact subsampFact, double qualityFactor,const char* outputFilePath){
    BMPFileLoader bmpFileLoader;

	if (!bmpFileLoader.load(srcFilePath)) {
        fputs("Malformed BMP file or unsupported BMP format.", stderr);
        return;
	}

    if (bmpFileLoader.height() > 0xFFFF || bmpFileLoader.width() > 0xFFFF) {
        fputs("The image size is too large.", stderr);
        return;
    }

    Matrix<YCbCr> ycbcrMat = Matrix<YCbCr>(bmpFileLoader.height(), bmpFileLoader.width());
    if (bmpFileLoader.getBitCount() == 24) {
        Matrix<RGBTriple> rgbMat = Matrix<RGBTriple>(bmpFileLoader.height(), bmpFileLoader.width());
        bmpFileLoader.readRGB(&rgbMat);
        ColorSpaceConverter::RGB2YCbCr(rgbMat, ycbcrMat);
        rgbMat.~Matrix();
    } else if (bmpFileLoader.getBitCount() == 32) {
        Matrix<RGBQuad> rgbMat = Matrix<RGBQuad>(bmpFileLoader.height(), bmpFileLoader.width());
        bmpFileLoader.readRGB(&rgbMat);
        ColorSpaceConverter::RGB2YCbCr(rgbMat, ycbcrMat);
        rgbMat.~Matrix();
    }
    Encoder encoder((WORD)ycbcrMat.column_cnt, (WORD)ycbcrMat.row_cnt,subsampFact);
    encoder.makeMCUs(ycbcrMat);
    ycbcrMat.~Matrix();
    encoder.doFDCT();
    encoder.quantize(qualityFactor);
    encoder.encode();
    encoder.makeJPGFile(outputFilePath);
}

void decode(const char* srcFilePath, const char* outputFilePath) {
    JPEGFileLoader loader;
    if (!loader.load(srcFilePath)) {
        fputs("Malformed JPEG file or unsupported JPEG format.", stderr);
        return;
    }
    
    Decoder decoder;
    decoder.setComponentConfigs(loader.getComponentConfigs());
    decoder.setSize(loader.getWidth(), loader.getHeight());
    const std::vector<std::pair<int, HuffmanTable>>(&huffTables)[(int)HTableType::MAXENUMVAL] = loader.getHuffmanTables();
    const std::vector<std::pair<int, BYTE[8][8]>>& qtables = loader.getQuantTables();
    for (const std::pair<int, BYTE[8][8]>& entry : qtables) {
        decoder.setQuantTable(entry.first, entry.second);
    }
    for (const std::pair<int, HuffmanTable>& entry : huffTables[(int)HTableType::AC]) {
        decoder.setSymbolTable(entry.first, entry.second, HTableType::AC);
    }

    for (const std::pair<int, HuffmanTable>& entry : huffTables[(int)HTableType::DC]) {
        decoder.setSymbolTable(entry.first, entry.second, HTableType::DC);
    }

    decoder.setCodedData(std::move(loader.getCodedData()));
    Matrix<YCbCr> ycbcrMat(loader.getHeight(), loader.getWidth());
    Matrix<RGBTriple> rgbMat(loader.getHeight(), loader.getWidth());
    decoder.decode();
    decoder.dequantize();
    decoder.doIDCT();
    decoder.makeYCbCrData(ycbcrMat);
    ColorSpaceConverter::YCbCr2RGB(ycbcrMat, rgbMat);
    ycbcrMat.~Matrix();
    BMPFileBuilder builder;
    builder.setHeight(loader.getHeight())
        .setWidth(loader.getWidth())
        .setRGBData(rgbMat)
        .makeBMPFile(outputFilePath);
}

int main(int argc, char** argv) {
    if (!initialize(argc, argv)) {
        fputs("Illegal argument(s).", stderr);
        putchar('\n');
        printHelp();
    }

    switch (mode) {
    case WORKING_MODE::ENCODE:
        encode(src, subsampFact, qualityFactor, dst);
        break;
    case WORKING_MODE::DECODE:
        decode(src, dst);
        break;
    }
    return 0;
}