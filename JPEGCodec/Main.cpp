#include "Encoder.h"
#include "UtilFunc.h"
#include "BMPFile.h"

static BMPFile bmpFile;
static const char* src;
static const char* dst;
static BYTE subsampFact_H, subsampFact_V;
static SubsampFact subsampFact;
static bool mean_samp_on;

static bool initialize(int argc, char** argv) {
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

int main9(int argc, char** argv) {
	initialize(argc, argv);
	return 0;
}