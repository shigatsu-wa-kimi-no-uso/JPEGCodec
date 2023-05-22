#include<stdio.h>
#include "ColorSpaceConverter.h"
#include "BMPFile.h"
#include "Encoder.h"
#include "UtilFunc.h"
#include "DCT.h"


BMPFile bmpFile;
const char* src;
const char* dst;
BYTE subsampFact_H, subsampFact_V;
SubsampFact subsampFact;
float qualityFactor;

const char* opt_fact_h = "-fact_h";
const char* opt_fact_v = "-fact_v";
const char* opt_src = "-src";
const char* opt_dst = "-dst";
const char* opt_quality = "-quality";
const char* opt_help = "-h";

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
    if (testopt(argc, argv, opt_help)) {
        return false;
    }
    const char* fact_h = getoptarg(argc, argv, opt_fact_h);
    const char* fact_v = getoptarg(argc, argv, opt_fact_v);
    src = getoptarg(argc, argv, opt_src);
    dst = getoptarg(argc, argv, opt_dst);
    const char* szQualityFactor = getoptarg(argc, argv, opt_quality);
    if (src == nullptr || dst == nullptr || fact_h == nullptr || fact_v == nullptr || szQualityFactor == nullptr) {
        fprintf(stderr, "Illegal argument.\n");
        return false;
    }

    subsampFact.factor_h = atoi(fact_h);
    subsampFact.factor_v = atoi(fact_v);
    qualityFactor = atof(szQualityFactor);
    if ((int)subsampFact.factor_h < 0 || (int)subsampFact.factor_v < 0) {
        fprintf(stderr, "Illegal argument.\n");
        return false;
    }
#ifdef DEBUG
    printf("fact_h: %d fact_v: %d\n", (int)subsampFact.factor_h, (int)subsampFact.factor_v);
#endif
    if (!bmpFile.load(src)) {
        fprintf(stderr,"Malformed BMP hFile or unsupported BMP format.\n");
        return false;
    }
    return true;
}

void encode(BMPFile bmpFile, SubsampFact subsampFact, const char* outputFilePath)
{
    ColorSpaceConverter csc;

    Matrix<YCbCr> ycbcrMat = Matrix<YCbCr>(bmpFile.height(), bmpFile.width());

    if (bmpFile.getBitCount() == 24) {
        Matrix<RGBTriple> rgbMat = Matrix<RGBTriple>(bmpFile.height(), bmpFile.width());
        bmpFile.readRGB(&rgbMat);
        csc.RGB2YCbCr(&rgbMat, &ycbcrMat);
        rgbMat.~Matrix();
    } else if (bmpFile.getBitCount() == 32) {
        Matrix<RGBQuad> rgbMat = Matrix<RGBQuad>(bmpFile.height(), bmpFile.width());
        bmpFile.readRGB(&rgbMat);
        csc.RGB2YCbCr(&rgbMat, &ycbcrMat);
        rgbMat.~Matrix();
    }
    Encoder encoder;
    encoder.makeMCUs(ycbcrMat, subsampFact);
    encoder.doFDCT();
    encoder.quantize(qualityFactor);
    encoder.encode();
    encoder.makeJPGFile(outputFilePath);
}

int main(int argc, char** argv) {
    if (initialize(argc, argv)) {
        encode(bmpFile, subsampFact, dst);
    } else {
        printHelp();
    }
    return 0;
}