#include<stdio.h>
#include "ColorSpaceConverter.h"
#include "BMPFileLoader.h"
#include "JPEGFileLoader.h"
#include "Encoder.h"
#include "Decoder.h"
#include "UtilFunc.h"



const char* src;
const char* dst;
SubsampFact subsampFact;
float qualityFactor;
const char* opt_fact_h = "-fact_h";
const char* opt_fact_v = "-fact_v";
const char* opt_src = "-src";
const char* opt_dst = "-dst";
const char* opt_quality = "-quality";
const char* opt_help = "-h";
const char* opt_encode = "-encode";
const char* opt_decode = "-decode";

enum WORKING_MODE {
    UNDEFINED,
    ENCODE,
    DECODE
}mode;

void printHelp() {
    char buf[256];
    puts("Usage:");
    sprintf(buf, "%s [arg]", opt_fact_h);
    printf("%24s %s\n", buf, "Specifies horizontal subsampling factor by an integer [arg]. 1<= [arg] <= 2");
    sprintf(buf, "%s [arg]", opt_fact_v);
    printf("%24s %s\n", buf, "Specifies vertical subsampling factor by an integer [arg]. 1<= [arg] <= 2");
    sprintf(buf, "%s [arg]", opt_src);
    printf("%24s %s\n", buf, "Specifies a BMP file to convert to JPG format.");
    sprintf(buf, "%s [arg]", opt_dst);
    printf("%24s %s\n", buf, "Creates the JPG file with the specified file path.");
    sprintf(buf, "%s [arg]", opt_quality);
    printf("%24s %s\n", buf, "Specifies a quality factor. 0 < [arg] <= 100");
    printf("%24s %s\n", opt_help, "Shows this info.");

}

bool initialize(int argc, char** argv) {
    mode = UNDEFINED;
    if (testopt(argc, argv, opt_help)) {
        printHelp();
        return true;
    }

    if (testopt(argc, argv, opt_encode)) {
        mode = ENCODE;
    } else if (testopt(argc, argv, opt_decode)) {
        mode = DECODE;
    } else {
        return false;
    }

    const char* fact_h = getoptarg(argc, argv, opt_fact_h);
    const char* fact_v = getoptarg(argc, argv, opt_fact_v);
    src = getoptarg(argc, argv, opt_src);
    dst = getoptarg(argc, argv, opt_dst);
    const char* szQualityFactor = getoptarg(argc, argv, opt_quality);
    if (src == nullptr || dst == nullptr || fact_h == nullptr || fact_v == nullptr || szQualityFactor == nullptr) {
        return false;
    }

    subsampFact.factor_h = atoi(fact_h);
    subsampFact.factor_v = atoi(fact_v);
    qualityFactor = atof(szQualityFactor);
    if ((int)subsampFact.factor_h < 0 || (int)subsampFact.factor_v < 0) {
        return false;
    }
#ifdef DEBUG
    printf("fact_h: %d fact_v: %d\n", (int)subsampFact.factor_h, (int)subsampFact.factor_v);
#endif
    return true;
}

void encode(const char* srcFilePath,SubsampFact subsampFact, float qualityFactor,const char* outputFilePath)
{
    BMPFileLoader bmpFileLoader;
    ColorSpaceConverter csc;
    Matrix<YCbCr> ycbcrMat = Matrix<YCbCr>(bmpFileLoader.height(), bmpFileLoader.width());
	if (!bmpFileLoader.load(srcFilePath)) {
        fputs("Malformed BMP file or unsupported BMP format.", stderr);
        return;
	}
    if (bmpFileLoader.getBitCount() == 24) {
        Matrix<RGBTriple> rgbMat = Matrix<RGBTriple>(bmpFileLoader.height(), bmpFileLoader.width());
        bmpFileLoader.readRGB(&rgbMat);
        csc.RGB2YCbCr(rgbMat, ycbcrMat);
        rgbMat.~Matrix();
    } else if (bmpFileLoader.getBitCount() == 32) {
        Matrix<RGBQuad> rgbMat = Matrix<RGBQuad>(bmpFileLoader.height(), bmpFileLoader.width());
        bmpFileLoader.readRGB(&rgbMat);
        csc.RGB2YCbCr(rgbMat, ycbcrMat);
        rgbMat.~Matrix();
    }
    Encoder encoder;
    encoder.makeMCUs(ycbcrMat, subsampFact);
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

    for (const std::pair<int, HuffmanTable>& entry : huffTables[(int)HTableType::AC]) {
        decoder.setSymbolTable(entry.first, entry.second, HTableType::AC);
    }

    for (const std::pair<int, HuffmanTable>& entry : huffTables[(int)HTableType::DC]) {
        decoder.setSymbolTable(entry.first, entry.second, HTableType::DC);
    }

    decoder.setCodedData(std::move(loader.getCodedData()));
    
    Matrix<YCbCr> ycbcrMat(loader.getHeight(), loader.getWidth());
    decoder.decode();
    decoder.dequantize();
    decoder.doIDCT();
    decoder.makeYCbCrData(ycbcrMat);
}

int main(int argc, char** argv) {
    if (!initialize(argc, argv)) {
        fputs("Illegal argument.", stderr);
        printHelp();
    }

    switch (mode)
    {
    case ENCODE:
        encode(src, subsampFact, qualityFactor, dst);
        break;
    case DECODE:
        decode(src, dst);
        break;
    }
    return 0;
}