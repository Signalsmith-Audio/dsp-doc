#include "fft.h"

// from the shared library
#include <complex>
#include <cmath>
#include <vector>
#include <test/tests.h>

#include "../common.h"

static std::vector<int> sizes() {
	std::vector<int> result;
	for (int i = 1; i < 16; ++i) {
		result.push_back(i);
	}
	for (int i = 16; i <= 65536; i *= 2) {
		result.push_back(i);
	}
	return result;
}

struct Errors {
	double rms, peak;
};

template<typename Sample>
Errors testComplexFft(Test &test, int size, Sample errorLimit=1e-5) {
	using complex = std::complex<Sample>;
	signalsmith::fft::FFT<Sample> fft(size);

	std::vector<complex> input(size), inputOriginal(size), output(size);
	double totalError2 = 0, totalExpected2 = 0;
	double peakError = 0;
	int totalCount = 0;

	// Pure tones
	auto testHarmonic = [&](int harmonic) {
		for (int i = 0; i < size; ++i) {
			Sample p = 2*M_PI*i*harmonic/size;
			input[i] = {std::cos(p), std::sin(p)};
		}
		inputOriginal = input;
		fft.fft(input, output);
		
		TEST_ASSERT(input == inputOriginal);
		for (int f = 0; f < size; ++f) {
			Sample expected = (f == harmonic) ? size : 0;
			double diff2 = std::norm(output[f] - expected);
			double diff = std::sqrt(diff2);
			totalError2 += diff2;
			totalExpected2 += std::norm(expected);
			totalCount++;
			peakError = std::max(peakError, diff);
			if (diff > errorLimit*size*std::sqrt(size)) {
				LOG_EXPR(fft.size())
				LOG_EXPR(size);
				LOG_EXPR(harmonic);
				LOG_EXPR(f);
				LOG_EXPR(diff);
				LOG_EXPR(output[f]);
				LOG_EXPR(expected);
				return test.fail("Harmonic failed");
			}
		}
	};
	if (size < 100) {
		for (int h = 0; h < size; ++h) testHarmonic(h);
	} else {
		for (int i = 0; i < 10; ++i) {
			int h = test.randomInt(0, size - 1);
			testHarmonic(h);
		}
	}
	double errorRms = std::sqrt(totalError2/totalCount);
	double expectedRms = std::sqrt(totalExpected2/totalCount);
	return {errorRms/expectedRms, peakError/expectedRms};
}

TEST("Complex FFT", complex_fft_double) {
	CsvWriter csv("fft-errors");
	csv.line("N", "float (RMS)", "float (peak)", "float (RMS theoretical)", "double (RMS)", "double (peak)", "double (RMS theoretical)");
	for (auto size : sizes()) {
		auto doubleError = testComplexFft<double>(test, size, 1e-10);
		if (!test.success) return;
		auto floatError = testComplexFft<float>(test, size, 1e-4);
		if (!test.success) return;
		
		double floatE = 5.96046448e-8, doubleE = 1.110223e-16;
		double gamma = 1;
		double floatExpectedRms = floatE*((3 + std::sqrt(2) + 2*gamma)*std::log2(size) - (3 + 2*gamma));
		double doubleExpectedRms = doubleE*((3 + std::sqrt(2) + 2*gamma)*std::log2(size) - (3 + 2*gamma));
		csv.line(size, floatError.rms, floatError.peak, floatExpectedRms, doubleError.rms, doubleError.peak, doubleExpectedRms);
	}
}
