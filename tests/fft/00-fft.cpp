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
		result.push_back(i*5/4);
		result.push_back(i*3/2);
	}
	return result;
}

struct Errors {
	double rms, peak;
};

template<typename Sample>
Errors testComplexHarmonics(Test &test, int size, Sample errorLimit) {
	using complex = std::complex<Sample>;
	signalsmith::fft::FFT<Sample> fft(size);

	std::vector<complex> input(size), inputOriginal(size), output(size);
	double totalError2 = 0, totalExpected2 = 0;
	double peakError = 0;
	int totalCount = 0;

	// Pure tones
	auto testHarmonic = [&](int harmonic) {
		for (int i = 0; i < size; ++i) {
			// Wrap the phase around as ints, otherwise we accumulate floating-point errors later in the buffer
			long phaseIndex = (long(i)*harmonic)%long(size);
			Sample p = 2*M_PI*phaseIndex/size;
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
		for (int i = 0; i < 25; ++i) {
			int h = test.randomInt(0, size - 1);
			testHarmonic(h);
		}
	}
	
	double errorRms = std::sqrt(totalError2/totalCount);
	double expectedRms = std::sqrt(totalExpected2/totalCount);
	return {errorRms/expectedRms, peakError/expectedRms};
}

template<typename Sample>
void testComplexLinearity(Test &test, int size, Sample errorLimit) {
	using complex = std::complex<Sample>;
	std::vector<complex> inputA(size), outputA(size);
	std::vector<complex> inputB(size), outputB(size);
	std::vector<complex> inputSum(size), outputSum(size);
	
	for (int i = 0; i < size; ++i) {
		inputA[i] = complex{Sample(test.random(-10, 10)), Sample(test.random(-10, 10))};
		inputB[i] = complex{Sample(test.random(-10, 10)), Sample(test.random(-10, 10))};
		inputSum[i] = inputA[i] + inputB[i];
	}

	signalsmith::fft::FFT<Sample> fft(size);
	TEST_ASSERT((int)fft.size() == size);
	fft.fft(inputA, outputA.data());
	fft.fft((const complex *)inputB.data(), outputB);
	fft.fft(inputSum.data(), outputSum.data());

	for (int f = 0; f < size; ++f) {
		complex expected = outputSum[f];
		complex sum = outputA[f] + outputB[f];
		
		double diff = std::abs(expected - sum);
		double diffRatio = diff/std::max(std::abs(expected), std::abs(sum));

		if (diffRatio > errorLimit*std::sqrt(size)) {
			LOG_EXPR(size);
			LOG_EXPR(f);
			LOG_EXPR(diff);
			return test.fail("Linearity failed");
		}
	}
}

template<typename Sample>
Errors testComplexFft(Test &test, int size, Sample errorLimit=1e-5) {
	auto errors = testComplexHarmonics(test, size, errorLimit);
	if (test.success) testComplexLinearity(test, size, errorLimit*100);
	return errors;
}

TEST("Complex FFT") {
	CsvWriter csvRms("fft-errors-rms");
	CsvWriter csvPeak("fft-errors-peak");
	csvRms.line("N", "measured (float)", "measured (double)", "limit (float)", "limit (double)");
	csvPeak.line("N", "measured (float)", "measured (double)", "limit (float)", "limit (double)");
	for (auto size : sizes()) {
		auto doubleError = testComplexFft<double>(test, size, 1e-12);
		if (!test.success) return;
		auto floatError = testComplexFft<float>(test, size, 1e-6);
		if (!test.success) return;
		
		double floatE = 5.96046448e-8, doubleE = 1.110223e-16;
		double gamma = 2;
		double floatExpectedRms = floatE*((3 + std::sqrt(2) + 2*gamma)*std::log2(size) - (3 + 2*gamma));
		double floatExpectedPeak = std::sqrt(size)*floatExpectedRms;
		double doubleExpectedRms = doubleE*((3 + std::sqrt(2) + 2*gamma)*std::log2(size) - (3 + 2*gamma));
		double doubleExpectedPeak = std::sqrt(size)*doubleExpectedRms;
		csvRms.line(size, floatError.rms, doubleError.rms, floatExpectedRms, doubleExpectedRms);
		csvPeak.line(size, floatError.peak, doubleError.peak, floatExpectedPeak, doubleExpectedPeak);
	}
}

template<typename Sample, bool modified=false>
void testRealFft(Test &test, int size, Sample errorLimit=1e-5) {
	TEST_ASSERT(size%2 == 0); // only even sizes are valid

	using complex = std::complex<Sample>;
	std::vector<complex> inputComplex(size), outputComplex(size), outputReal(size);
	std::vector<Sample> inputReal(size);
	
	for (int i = 0; i < size; ++i) {
		inputComplex[i] = inputReal[i] = Sample(test.random(-10, 10));
	}
	for (int f = size/2; f < size; ++f) {
		outputReal[f] = f; // Known values, to prove we haven't messed them up
	}

	signalsmith::fft::RealFFT<Sample> realFft(size);
	signalsmith::fft::ModifiedRealFFT<Sample> mRealFft(size);
	if (modified) {
		// Rotate complex input backwards by 1/2 bin
		for (int i = 0; i < size; ++i) {
			double phase = M_PI*i/size;
			complex shift{Sample(std::cos(phase)), -Sample(std::sin(phase))};
			inputComplex[i] *= shift;
		}
		mRealFft.fft((const Sample *)inputReal.data(), outputReal.data());
	} else {
		realFft.fft((const Sample *)inputReal.data(), outputReal.data());
	}
	signalsmith::fft::FFT<Sample> fft(size);
	fft.fft(inputComplex, outputComplex);

	auto closeEnough = [&](complex expected, complex actual) {
		double diff = std::abs(expected - actual);
		double diffRatio = diff/std::max(std::abs(expected), std::abs(actual));
		return (diffRatio <= errorLimit*std::sqrt(size));
	};
	
	if (!modified) {
		if (!closeEnough(outputComplex[0], outputReal[0].real())) {
			return test.fail("real[0].real() is f=0");
		}
		if (!closeEnough(outputComplex[size/2], outputReal[0].imag())) {
			return test.fail("real[0].imag() is Nyquist");
		}
	}

	for (int f = (modified ? 0 : 1); f < size/2; ++f) {
		if (!closeEnough(outputComplex[f], outputReal[f])) {
			LOG_EXPR(size);
			LOG_EXPR(f);
			LOG_EXPR(outputComplex[f]);
			LOG_EXPR(outputReal[f]);
			return test.fail("Real failed");
		}
	}
	// Should have left the second half of the array alone
	for (int f = size/2; f < size; ++f) {
		TEST_ASSERT(outputReal[f] == Sample(f));
	}
}

TEST("Real FFT") {
	for (auto size : sizes()) {
		if (size%2) continue; // even sizes only

		testRealFft<double, false>(test, size, 1e-12);
		if (!test.success) return;
		testRealFft<float, false>(test, size, 1e-5);
		if (!test.success) return;
	}
}

TEST("Modified Real FFT") {
	for (auto size : sizes()) {
		if (size%2) continue; // even sizes only

		testRealFft<double, true>(test, size, 1e-12);
		if (!test.success) return;
		testRealFft<float, true>(test, size, 1e-5);
		if (!test.success) return;
	}
}

TEST("sizeMinimum/sizeMaximum") {
	using Sample = float;
	
	auto testPowers = [&](int size) {
		if (size <= 0) return test.fail("size cannot be 0");
		while (size%2 == 0) size /= 2;
		while (size%3 == 0) size /= 3;
		TEST_ASSERT(size == 1);
	};

	for (auto maxSize : sizes()) {
		int size = test.random(1, maxSize);
		
		{
			signalsmith::fft::FFT<Sample> fft(size, -1);
			TEST_ASSERT(fft.size() == signalsmith::fft::FFT<Sample>::fastSizeBelow(size));
			TEST_ASSERT((int)fft.size() <= size);
			testPowers(fft.size());

			if (size > 1) {
				signalsmith::fft::RealFFT<Sample> realFft(size, -1);
				TEST_ASSERT(realFft.size() == signalsmith::fft::RealFFT<Sample>::fastSizeBelow(size));
				TEST_ASSERT((int)realFft.size() <= size);
				testPowers(realFft.size());

				signalsmith::fft::ModifiedRealFFT<Sample> mRealFft(size, -1);
				TEST_ASSERT(mRealFft.size() == signalsmith::fft::ModifiedRealFFT<Sample>::fastSizeBelow(size));
				TEST_ASSERT((int)mRealFft.size() <= size);
				testPowers(mRealFft.size());
			}
		}
		{
			signalsmith::fft::FFT<Sample> fft(size, 1);
			TEST_ASSERT(fft.size() == signalsmith::fft::FFT<Sample>::fastSizeAbove(size));
			TEST_ASSERT((int)fft.size() >= size);
			testPowers(fft.size());

			if (size > 1) {
				signalsmith::fft::RealFFT<Sample> realFft(size, 1);
				TEST_ASSERT(realFft.size() == signalsmith::fft::RealFFT<Sample>::fastSizeAbove(size));
				TEST_ASSERT((int)realFft.size() >= size);
				testPowers(realFft.size());

				signalsmith::fft::ModifiedRealFFT<Sample> mRealFft(size, 1);
				TEST_ASSERT(mRealFft.size() == signalsmith::fft::ModifiedRealFFT<Sample>::fastSizeAbove(size));
				TEST_ASSERT((int)mRealFft.size() >= size);
				testPowers(mRealFft.size());
			}
		}
	}
}
