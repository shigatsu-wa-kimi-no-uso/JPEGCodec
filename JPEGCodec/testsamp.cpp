#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "ColorSpaceConverter.h"
#include "DCTBlockSplitter.h"
#include "DCT.h"
#include "BMPFile.h"
#include "Downsampler.h"
#include "Windows.h"
#include "Encoder.h"
#include <thread>
#include <mutex>
#include <semaphore>
#include "IntHuffman.h"
#include <iomanip>
#include <bitset>
#include "CodingUtil.h"
#include "UtilFunc.h"

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
BYTE subsampFact_H, subsampFact_V;
SubsampFact subsampFact;
bool mean_samp_on;



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
     //   bmpFile.readRGB(&rgbMat, y_start, y_end);
        csc.RGB2YCbCr(&rgbMat, &ycbcrMat);
        rgbMat.~Matrix();
    } else if (bmpFile.getBitCount() == 32) {
        Matrix<RGBQuad> rgbMat = Matrix<RGBQuad>(thisBlockHeight, bmpFile.width());
     //   bmpFile.readRGB(&rgbMat, y_start, y_end);
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

bool initialize(int argc, char** argv) {
    const char* fact_h = getoptarg(argc, argv, "-fact_h");
    const char* fact_v = getoptarg(argc, argv, "-fact_v");
    src = getoptarg(argc, argv, "-src");
    dst = getoptarg(argc, argv, "-dst");

    if (src == nullptr || dst == nullptr || fact_h == nullptr || fact_v == nullptr) {
        return false;
    }

    subsampFact.factor_h = atoi(fact_h);
    subsampFact.factor_v = atoi(fact_v);
    mean_samp_on = testopt(argc, argv, "-mean_samp");

    printf("fact_h: %d fact_v: %d\n", (int)subsampFact.factor_h, (int)subsampFact.factor_v);

    if (mean_samp_on) {
        printf("Option -mean_samp: true\n");
    } else {
        printf("Option -mean_samp: false\n");
    }

    if (!bmpFile.load(src)) {
        perror("Malformed BMP file or unsupported BMP format.");
    }
}


void print(std::vector<IntHuffman::CanonicalTableEntry>& table,size_t freqs[]) {
    int width = table.back().codeLength + 3;
	std::cout << std::left << std::setw(width) << "len" << std::left << std::setw(10) << "val" << std::left << std::setw(10) << "freq" << "\n";
	for (IntHuffman::CanonicalTableEntry& entry:table) {
		std::cout << std::left << std::setw(width) <<entry.codeLength << std::left << std::setw(10) << entry.val << std::left << std::setw(10) << freqs[entry.val] << "\n";
	}
}

void print(std::vector<IntHuffman::TableEntry>& table, size_t freqs[]) {
	int width = table.back().bits.length() + 3;
	std::cout << std::left << std::setw(width) << "bits" << std::left << std::setw(10) << "val" << std::left << std::setw(10) << "freq" << "\n";
	for (IntHuffman::TableEntry& entry : table) {
		std::cout << std::left << std::setw(width) << entry.bits << std::left << std::setw(10) << entry.val << std::left << std::setw(10) << freqs[entry.val] << "\n";
	}
}


void print(std::vector<BitString>& bs, std::vector<IntHuffman::CanonicalTableEntry>& table, size_t freqs[]) {
    int width = bs.back().length() + 3;
    std::cout << std::left << std::setw(width) << "bits" << std::left << std::setw(10) << "val" << std::left << std::setw(10) << "freq" << "\n";
    int i = 0;
    for (BitString& b : bs) {
        std::cout << std::left << std::setw(width) << b << std::left << std::setw(10) << table[i].val << std::left << std::setw(10) << freqs[table[i].val] << "\n";
        i++;
    }
}

int main0() {
    return 0;
}

int main(int argc, char** argv) {
	/*DWORD zigzag[8][8] = {
		0,1,5,6,14,15,27,28,
		2,4,7,13,16,26,29,42,
		3,8,12,17,25,30,41,43,
		9,11,18,24,31,40,44,53,
		10,19,23,32,39,45,52,54,
		20,22,33,38,46,51,55,60,
		21,34,37,47,50,56,59,61,
		35,36,48,49,57,58,62,63
	};
    Block block;
    int zigzagged[64]{ 35,7,0,0,0,-6,-2,0,0,-9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,0 };
    for (int r=0;r<8;++r){
        for (int c = 0; c < 8; c++) {
            block[r][c] = zigzagged[zigzag[r][c]];
            printf("%2.0f ", block[r][c]);
        }
        printf("\n");
    }
    printf("\n");
    int codes[64];
    RLE::getRLECodes(&block, codes);
    for (int i = 0; i < 32; i+=2) {
        if (codes[i] == 0 && codes[i + 1] == 0) {
            printf("(EOB)");
            break;
        } else {
            printf("(%d,%d)", codes[i], codes[i + 1]);
        }

    }
    printf("\n");*/
    
    IntHuffman bh;
	char a[] = "The encoding procedure is defined in terms of a set of extended tables, XHUFCO and XHUFSI, which contain the "
		"complete set of Huffman codes and sizes for all possible difference values.For full 12 - bit precision the tables are relatively "
		"large.For the baseline system, however, the precision of the differences may be small enough to make this description "
		"practical";
    size_t freq[256] = { 0 };
    std::vector<IntHuffman::CanonicalTableEntry> table;
    std::vector<IntHuffman::TableEntry> table2;
    std::vector<int> lengthCnt;
    for (int i = 0; i < 365; ++i) {
        freq[a[i]]++;
    }
    bh.setFrequencyMap(&freq);
    bh.buildTree();
    bh.getTable(table2);
    print(table2, freq);
    bh.getCanonicalTable(table);
    bh.getCanonicalTable(lengthCnt,7);
    print(table, freq);
    std::vector<BitString> bs,bs2;
    bh.getCanonicalCodes(table, bs);
    print(bs, table, freq);
    bh.getCanonicalCodes(lengthCnt, bs2);
    bs2.erase(bs2.begin());
    print(bs2, table, freq);
    /*
    CodingUnits units;
    initialize(argc, argv);
    ColorSpaceConverter csc;
    Matrix<YCbCr> ycbcrMat = Matrix<YCbCr>(bmpFile.height(), bmpFile.width());
    units.makeMCUs(&ycbcrMat, subsampFact);  */
    return 0;
}

int main3(int argc, char** argv)
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

