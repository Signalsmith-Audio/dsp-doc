// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "fft.h"
#include "../common.h"

#include "./filter-tests.h"

template<typename Sample>
void testBandpass(Test &&test, double freq, double octaves=1.2) {
	signalsmith::filters::BiquadStatic<Sample, false> filter;
	
	filter.bandpass(freq, octaves, true);
	Spectrum spectrum = getSpectrum(filter);
	
	double criticalDb = ampToDb(interpSpectrum(spectrum, freq));
	if (std::abs(criticalDb) > 0.001) {
		test.log(criticalDb);
		return test.fail("0 at critical");
	}
	if (std::abs(spectrum[0]) > 0.001) {
		writeSpectrum(spectrum, "fail-spectrum");
		return test.fail("spectrum[0] == 0: ", spectrum[0]);
	}
	int nyquistIndex = spectrum.size()/2;
	if (std::abs(spectrum[nyquistIndex]) > 0.001) {
		writeSpectrum(spectrum, "fail-spectrum");
		return test.fail("spectrum[Nyquist] == 0: ", spectrum[0]);
	}
	if (!isMonotonic(spectrum, 0, freq)) {
		writeSpectrum(spectrum, "fail-spectrum");
		return test.fail("monotonic below");
	}
	if (!isMonotonic(spectrum, 0.5, freq)) {
		writeSpectrum(spectrum, "fail-spectrum");
		return test.fail("monotonic above");
	}

	double bandLowFreq = freq*std::pow(0.5, octaves/2);
	double db = ampToDb(interpSpectrum(spectrum, bandLowFreq, -1));
	if (db > -3.01029995664) {
		writeSpectrum(spectrum, "fail-spectrum");
		return test.fail("lower bandwidth boundary: ", db, " below f=", bandLowFreq);
	}
	db = ampToDb(interpSpectrum(spectrum, bandLowFreq, 1));
	if (db < -3.01029995664) {
		writeSpectrum(spectrum, "fail-spectrum");
		return test.fail("lower bandwidth boundary: ", db, " above f=", bandLowFreq);
	}
}

TEST("Bandpass", filters_bandpass) {
//	testBandpass<double>(test.prefix("double@0.05"), 0.05);
//	if (test.success) testBandpass<float>(test.prefix("float@0.05"), 0.05);
	if (test.success) testBandpass<double>(test.prefix("double@0.2"), 0.2);
	if (test.success) testBandpass<float>(test.prefix("float@0.2"), 0.2);
}
