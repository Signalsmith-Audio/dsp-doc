// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "fft.h"
#include "../common.h"

#include "./filter-tests.h"

template<class Filter>
void testResponse(Test &test, Filter &filter, double accuracy=1e-6) {
	auto spectrum = getSpectrum(filter);
	for (size_t i = 0; i < spectrum.size()/2; ++i) {
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

TEST("Responses", filter_responses) {
	signalsmith::filters::BiquadStatic<double> filter;
	if (test.success) testResponse(test, filter);

	filter.lowpass(test.random(0, 0.5), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.highpass(test.random(0, 0.5), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.bandpass(test.random(0, 0.5), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.notch(test.random(0, 0.5), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.peak(test.random(0, 0.5), test.random(0.25, 4), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.highShelf(test.random(0, 0.5), test.random(0.25, 4), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);

	filter.lowShelf(test.random(0, 0.5), test.random(0.25, 4), test.random(0.5, 4));
	if (test.success) testResponse(test, filter);
}
