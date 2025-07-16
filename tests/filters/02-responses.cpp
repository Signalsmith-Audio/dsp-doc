// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "fft.h"
#include "../common.h"

#include "./filter-tests.h"

template<class Filter>
void testResponse(Test &test, Filter &filter, double accuracy=1e-6) {
	auto spectrum = getSpectrum(filter);
	
	for (int r = 0; r < 10; ++r) {
		int i = int(std::floor(test.random(0, spectrum.size()/2)));
		double f = i*1.0/spectrum.size();
		std::complex<double> predicted = filter.response(f);
		std::complex<double> actual = spectrum[i];
		
		TEST_ASSERT(std::abs(predicted - actual) < accuracy)
		
		double dbPredicted = filter.responseDb(f);
		double dbActual = 10*std::log10(std::norm(actual));
		if (dbPredicted > -100) {
			TEST_ASSERT(std::abs(dbPredicted - dbActual) < accuracy)
		}
	}
}

TEST("Responses") {
	signalsmith::filters::BiquadStatic<double> filter;
	if (test.success) testResponse(test, filter);

	filter.lowpass(test.random(0.01, 0.49), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.highpass(test.random(0.01, 0.49), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.bandpass(test.random(0.01, 0.49), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.notch(test.random(0.01, 0.49), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.peak(test.random(0.01, 0.49), test.random(0.25, 4), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.highShelf(test.random(0.01, 0.49), test.random(0.25, 4), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.lowShelf(test.random(0.01, 0.49), test.random(0.25, 4), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);
}

TEST("Copying response/coeffs") {
	signalsmith::filters::BiquadStatic<float> filterA, filterB;
	
	filterA.lowpassQ(0.2, 4);
	filterB(1);
	
	filterB.copyFrom(filterA);
	// Response is copied exactly
	for (float f = 0; f < 0.5; f += 0.01) {
		TEST_ASSERT(filterA.response(f) == filterB.response(f));
	}
	// But not the state
	TEST_ASSERT(filterA(0) != filterB(0));
}
