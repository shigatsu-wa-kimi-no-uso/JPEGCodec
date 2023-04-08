#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "ColorSpaceConverter.h"
#include "DCTBlockSplitter.h"
#include "DCT.h"
#include "BMPFile.h"
#include "Downsampler.h"
#include "Windows.h"
#include <thread>
#include <mutex>
#include <semaphore>

const char* getoptarg(int argc,char** argv,const char* opt) {
    
    for (int i = 0; i < argc - 1; ++i) {
        if (strcmp(argv[i], opt) == 0) {
            return argv[i + 1];
        }
    }
    return nullptr;
}

bool testopt(int argc, char** argv, const char* opt) {
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], opt) == 0) {
            return true;
        }
    }
    return false;
}


using Integer = int*;
using ref_Integer = Integer&;

void swap1(ref_Integer a,ref_Integer b) {
    Integer tmp = a;
    a = b;
    b = tmp;
}

//#define DEBUG

class Timer
{
public:
    unsigned long long clock;
	void start() {
		clock = GetTickCount64();
	}
    void start(const char* msg) {
        puts(msg);
        clock = GetTickCount64();
    }
	void pressAnyKeyAndStart() {
    #ifdef DEBUG
        getchar();
    #endif
		start();
	}
    void pressAnyKeyAndStart(const char* msg) {
    #ifdef DEBUG
        getchar();
    #endif
        start(msg);
    }
	void stop() {
		printf("%d ms\n", (int)(GetTickCount64() - clock));
	}
    void stop(const char* msg) {
        puts(msg);
        stop();
        
    }
};

BMPFile bmpFile;
const char* src;
const char* dst;
BYTE subsampFact_H;
BYTE subsampFact_V;
bool mean_samp_on;

int myround(float val) {
    int intval = val;
    if (val - intval >= 0.5f) {
        return intval + 1;
    } else {
        return intval;
    }
}

void writeComponent(Matrix<BYTE>* outputBuffer,FILE* hOutput, Matrix<float>* compMatrix, DWORD fileOffset) {
    fseek(hOutput, fileOffset, SEEK_SET);
    for (int i = 0; i < compMatrix->row_cnt; ++i) {
        for (int j = 0; j < compMatrix->column_cnt; ++j) {
            (*outputBuffer)[i][j] = myround((*compMatrix)[i][j]);
        }
        fwrite((*outputBuffer)[i], sizeof(BYTE), compMatrix->column_cnt, hOutput);
    }
}

void writeComponent(Matrix<BYTE>* outputBuffer, FILE* hOutput, Matrix<float>* compMatrix) {
    for (int i = 0; i < compMatrix->row_cnt; ++i) {
        for (int j = 0; j < compMatrix->column_cnt; ++j) {
            (*outputBuffer)[i][j] = myround((*compMatrix)[i][j]);
        }
        fwrite((*outputBuffer)[i], sizeof(BYTE), compMatrix->column_cnt, hOutput);
    }
}

//y从左下角开始
void encode2(LONG y_start, LONG y_end, int id) {
    // BOOL result = SetThreadAffinityMask(GetCurrentThread(), 1 << id);
    // printf("%d\n", result);
    LONG thisBlockHeight = y_end - y_start;
    FILE* hOutput = fopen(dst, "wb");
    ColorSpaceConverter csc;

    Matrix<YCbCr> ycbcrMat = Matrix<YCbCr>(thisBlockHeight, bmpFile.width());

    if (bmpFile.getBitCount() == 24) {
        Matrix<RGBTriple> rgbMat = Matrix<RGBTriple>(thisBlockHeight, bmpFile.width());
        bmpFile.readRGB(&rgbMat, y_start, y_end);
        csc.RGB2YCbCr(&rgbMat, &ycbcrMat);
        rgbMat.~Matrix();
    } else if (bmpFile.getBitCount() == 32) {
        Matrix<RGBQuad> rgbMat = Matrix<RGBQuad>(thisBlockHeight, bmpFile.width());
        bmpFile.readRGB(&rgbMat, y_start, y_end);
        csc.RGB2YCbCr(&rgbMat, &ycbcrMat);
        rgbMat.~Matrix();
    }

    Downsampler sampler;
    sampler.setChannelData(&ycbcrMat);
    sampler.setSubsamplingFactors(subsampFact_H, subsampFact_V);
    int col_sampcnt = sampler.getChromaColumnSampleCount();
    int row_sampcnt = sampler.getChromaRowSampleCount();
    Matrix<float> y = Matrix<float>(thisBlockHeight, bmpFile.width());
    Matrix<float> cb = Matrix<float>(row_sampcnt, col_sampcnt);
    Matrix<float> cr = Matrix<float>(row_sampcnt, col_sampcnt);

    if (mean_samp_on) {
        sampler.mean_sample(&y, &cb, &cr);
    } else {
        sampler.sample(&y, &cb, &cr);
    }
    ycbcrMat.~Matrix();

    int y_comp_size = bmpFile.height() * bmpFile.width() * sizeof(BYTE);
    int cb_comp_size = Downsampler::_getSampleCount(subsampFact_V, bmpFile.height()) * Downsampler::_getSampleCount(subsampFact_H, bmpFile.width()) * sizeof(BYTE);
    int thisblock_offset_y = (bmpFile.height() - y_end) * bmpFile.width() * sizeof(BYTE);
    int thisblock_offset_cbcr = Downsampler::_getSampleCount(subsampFact_V, bmpFile.height() - y_end) * Downsampler::_getSampleCount(subsampFact_H, bmpFile.width()) * sizeof(BYTE);

    DWORD output_file_offset = thisblock_offset_y;
    Matrix<BYTE> outputBuffer = Matrix<BYTE>(y.row_cnt, y.column_cnt);
    printf("y:[%d,%d) write y from offset %d \n", y_start, y_end, output_file_offset);
    writeComponent(&outputBuffer, hOutput, &y, output_file_offset);
    y.~Matrix();

    output_file_offset = y_comp_size + thisblock_offset_cbcr;
    printf("y:[%d,%d) write cb from offset %d \n", y_start, y_end, output_file_offset);
    writeComponent(&outputBuffer, hOutput, &cb, output_file_offset);
    cb.~Matrix();

    output_file_offset = y_comp_size +  cb_comp_size + thisblock_offset_cbcr;
    printf("y:[%d,%d) write cr from offset %d \n", y_start, y_end, output_file_offset);
    writeComponent(&outputBuffer, hOutput, &cr, output_file_offset);
    cr.~Matrix();

    fclose(hOutput);
}

void encode(BMPFile bmpFile, const char* outputFilePath, BYTE subsampFact_H,BYTE subsampFact_V, bool mean_samp_on) 
{
    FILE* hOutput = fopen(outputFilePath, "wb");
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

    Downsampler sampler;
    sampler.setChannelData(&ycbcrMat);
    sampler.setSubsamplingFactors(subsampFact_H, subsampFact_V);
    int col_sampcnt = sampler.getChromaColumnSampleCount();
    int row_sampcnt = sampler.getChromaRowSampleCount();
    Matrix<float> y = Matrix<float>(bmpFile.height(), bmpFile.width());
    Matrix<float> cb = Matrix<float>(row_sampcnt, col_sampcnt);
    Matrix<float> cr = Matrix<float>(row_sampcnt, col_sampcnt);

    if (mean_samp_on) {
        sampler.mean_sample(&y, &cb, &cr);
    } else {
        sampler.sample(&y, &cb, &cr);
    }
    ycbcrMat.~Matrix();

    Matrix<BYTE> outputBuffer = Matrix<BYTE>(y.row_cnt, y.column_cnt);
    writeComponent(&outputBuffer, hOutput, &y);
    y.~Matrix();

    writeComponent(&outputBuffer, hOutput, &cb);
    cb.~Matrix();

    writeComponent(&outputBuffer, hOutput, &cr);
    cr.~Matrix();

    fclose(hOutput);
}

int main(int argc, char** argv)
{
	std::ios::sync_with_stdio(false);
	const char* fact_h = getoptarg(argc, argv, "-fact_h");
	const char* fact_v = getoptarg(argc, argv, "-fact_v");
	src = getoptarg(argc, argv, "-src");
	dst = getoptarg(argc, argv, "-dst");

	if (src == nullptr || dst == nullptr || fact_h == nullptr || fact_v == nullptr) {
		return 0;
	}

	subsampFact_H = atoi(fact_h);
	subsampFact_V = atoi(fact_v);
	mean_samp_on = testopt(argc, argv, "-mean_samp");

    printf("fact_h: %d fact_v: %d\n", (int)subsampFact_H, (int)subsampFact_V);
    if (mean_samp_on) {
        printf("Option -mean_samp: true\n");
    } else {
        printf("Option -mean_samp: false\n");
    }

	if (!bmpFile.load(src)) {
		perror("Malformed BMP file or unsupported BMP format.");
	}

    encode(bmpFile, dst, subsampFact_H, subsampFact_V, mean_samp_on);

	return 0;
}

/*
    int thread_count = 5;
    LONG subblock_y_len = (bmpFile.height() / thread_count) & ~1;
    LONG subblock_y_start = 0;
    std::thread* threads = new std::thread[thread_count];
    for (int i = 0; i < thread_count - 1; ++i) {
        threads[i] = std::thread(encode, subblock_y_start, subblock_y_start + subblock_y_len,i);
        subblock_y_start += subblock_y_len;
    }

    threads[thread_count - 1] = std::thread(encode, subblock_y_start, bmpFile.height(), thread_count - 1);
    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }
    delete[] threads;
*/

int main2(int argc,char** argv)
{
    std::ios::sync_with_stdio(false);
	const char* fact_h = getoptarg(argc, argv, "-fact_h");
	const char* fact_v = getoptarg(argc, argv, "-fact_v");
	src = getoptarg(argc, argv, "-src");
	dst = getoptarg(argc, argv, "-dst");

	if (src == nullptr || dst == nullptr|| fact_h == nullptr || fact_v == nullptr) {
		return 0;
	}

	subsampFact_H = atoi(fact_h);
	subsampFact_V = atoi(fact_v);



    Timer timer2;
    timer2.start();
    Timer timer;
    
   

    if (!bmpFile.load(src)) {
        perror("Malformed BMP file or unsupported BMP format.");
    }

    timer.pressAnyKeyAndStart("allocate rgb buffer:");

    Matrix<RGBTriple> rgbMat = Matrix<RGBTriple>(bmpFile.height(), bmpFile.width());

    timer.stop();

    timer.pressAnyKeyAndStart("read rgb:");

    timer.stop();
   // bool res = bmp.readRGB(&rgbMat);

//    bmp.close();

    ColorSpaceConverter csc;

    Matrix<YCbCr> ycbcrMat = Matrix<YCbCr>(bmpFile.height(), bmpFile.width());

    timer.pressAnyKeyAndStart("rgb2ycbcr:");
    csc.RGB2YCbCr(&rgbMat, &ycbcrMat);
    timer.stop();
    rgbMat.~Matrix();



    Downsampler dsa;
 
    dsa.setChannelData(&ycbcrMat);
    dsa.setSubsamplingFactors(subsampFact_H, subsampFact_V);


    timer.pressAnyKeyAndStart("allocate y,cb,cr matrices:");
    Matrix<float> y = Matrix<float>(bmpFile.height(), bmpFile.width());
    Matrix<float>  cb = Matrix<float>(bmpFile.height() / subsampFact_V, bmpFile.width() / subsampFact_H);
    Matrix<float>  cr = Matrix<float>(bmpFile.height() / subsampFact_V, bmpFile.width() / subsampFact_H);
    timer.stop();

    timer.pressAnyKeyAndStart("samp:");
    if (testopt(argc, argv, "-mean_samp")) {
        dsa.mean_sample(&y, &cb, &cr);
    } else {
        dsa.sample(&y, &cb, &cr);
    }
    timer.stop();
    ycbcrMat.~Matrix();

    timer.pressAnyKeyAndStart("save y comp:");

    FILE* out = fopen(dst, "wb");
    for (int i = 0; i < y.row_cnt; ++i) {
        for (int j = 0; j < y.column_cnt; ++j) {
            BYTE val = round(y[i][j]);
            fwrite(&val, 1, 1, out);
        }
    }
    y.~Matrix();

    timer.stop();
    timer.pressAnyKeyAndStart("save cb comp:");
    for (int i = 0; i < cb.row_cnt; ++i) {
        for (int j = 0; j < cb.column_cnt; ++j) {
            BYTE val = round(cb[i][j]);
            fwrite(&val, 1, 1, out);
        }
    }
    cb.~Matrix();

    timer.stop();
    timer.pressAnyKeyAndStart("save cr comp:");
    for (int i = 0; i < cr.row_cnt; ++i) {
        for (int j = 0; j < cr.column_cnt; ++j) {
            BYTE val = round(cr[i][j]);
            fwrite(&val, 1, 1, out);
        }
    }
    fclose(out);
    timer.stop();
    timer2.stop();
    return 0;
}

