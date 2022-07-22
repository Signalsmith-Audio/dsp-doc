// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "fft.h"
#include "../common.h"

template<typename Sample>
void testAllpass(Test &&test, double freq, double octaves=1.2) {
	signalsmith::filters::BiquadStatic<Sample, false> filter;
	float accuracy = 1e-4;
	
	filter.allpass(freq, octaves);
	
	for (int r = 0; r < 10; ++r) {
		double f = test.random(0, 0.5);
		auto db = filter.responseDb(f);
		TEST_APPROX(db, 0, accuracy);
	}
	
	// Phase is 180deg at critical freq
	auto critical = filter.response(freq);
	TEST_APPROX(critical, Sample(-1), accuracy);

	if (freq < 0.1) {
		auto lowFreq = freq*std::pow(0.5, octaves*0.5);
		std::complex<Sample> expected{0, -1};
		auto actual = filter.response(lowFreq);
		TEST_APPROX(actual, expected, accuracy);
	}
	auto highFreq = freq*std::pow(2, octaves*0.5);
	if (highFreq < 0.02) {
		std::complex<Sample> expected{0, 1};
		auto actual = filter.response(highFreq);
		TEST_APPROX(actual, expected, accuracy);
	}
}

TEST("Allpass", filters_allpass) {
	for (int r = 0; r < 10; ++r) {
		double freq = test.random(0.01, 0.49);
		if (test.success) testAllpass<double>(test.prefix("double@" + std::to_string(freq)), freq);
		if (test.success) testAllpass<float>(test.prefix("float@" + std::to_string(freq)), freq);
	}
}
