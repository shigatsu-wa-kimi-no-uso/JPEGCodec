#include<stdio.h>
#include "ColorSpaceConverter.h"
#include "BMPFile.h"
#include "Encoder.h"
#include "UtilFunc.h"

BMPFile bmpFile;
const char* src;
const char* dst;
BYTE subsampFact_H, subsampFact_V;
SubsampFact subsampFact;
bool mean_samp_on;
float qualityFactor;

bool initialize(int argc, char** argv) {
    const char* fact_h = getoptarg(argc, argv, "-fact_h");
    const char* fact_v = getoptarg(argc, argv, "-fact_v");
    src = getoptarg(argc, argv, "-src");
    dst = getoptarg(argc, argv, "-dst");
    const char* szQualityFactor = getoptarg(argc, argv, "-quality");
    if (src == nullptr || dst == nullptr || fact_h == nullptr || fact_v == nullptr || szQualityFactor == nullptr) {
        return false;
    }

    subsampFact.factor_h = atoi(fact_h);
    subsampFact.factor_v = atoi(fact_v);
    qualityFactor = atof(szQualityFactor);
    mean_samp_on = testopt(argc, argv, "-mean_samp");

    printf("fact_h: %d fact_v: %d\n", (int)subsampFact.factor_h, (int)subsampFact.factor_v);

    if (mean_samp_on) {
        printf("Option -mean_samp: true\n");
    } else {
        printf("Option -mean_samp: false\n");
    }

    if (!bmpFile.load(src)) {
        perror("Malformed BMP hFile or unsupported BMP format.");
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
    encoder.doQuantize(qualityFactor);
    encoder.encode();
    encoder.makeJPGFile(outputFilePath);
}

int main(int argc, char** argv) {
    if (!initialize(argc, argv)) {
        perror("illegal command.");
    }
    encode(bmpFile, subsampFact, dst);
    return 0;
}