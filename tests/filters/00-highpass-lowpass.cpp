// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "fft.h"
#include "../common.h"

#include "./filter-tests.h"

template<typename Sample>
void testReset(Test &&test) {
	signalsmith::filters::BiquadStatic<Sample> filter;
	filter.lowpass(0.25);
	
	constexpr int length = 10;
	std::vector<Sample> outA(length), outB(length), outNoReset(length);

	filter(1e10);
	for (int i = 0; i < length; ++i) {
		outNoReset[i] = filter(i);
	}
	filter.reset();
	for (int i = 0; i < length; ++i) {
		outA[i] = filter(i);
	}
	filter(1e10);
	filter.reset();
	for (int i = 0; i < length; ++i) {
		outB[i] = filter(i);
	}
	
	if (outA != outB) return test.fail("Reset didn't clear properly");
	if (outA == outNoReset) return test.fail("something weird's going on");
}
TEST("Filter reset", filter_reset) {
	testReset<double>(test.prefix("double"));
	testReset<float>(test.prefix("float"));
}

// Should be Butterworth when we don't specify a bandwidth
template<typename Sample>
void testButterworth(Test &&test, double freq, signalsmith::filters::BiquadDesign design=signalsmith::filters::BiquadDesign::bilinear) {
	signalsmith::filters::BiquadStatic<Sample> filter;
	
	double zeroIsh = 1e-5; // -100dB
	std::complex<double> one = 1;
	bool isBilinear = (design == signalsmith::filters::BiquadDesign::bilinear);
	
	{
		filter.lowpass(freq, design);
		auto spectrum = getSpectrum(filter);
		int nyquistIndex = (int)spectrum.size()/2;

		if (std::abs(spectrum[0] - one) > zeroIsh) return test.fail("1 at 0: ", spectrum[0]);

		// -3dB at critical point
		double criticalDb = ampToDb(interpSpectrum(spectrum, freq));
		double expectedDb = ampToDb(std::sqrt(0.5));
		double difference = std::abs(criticalDb - expectedDb);
		if (isBilinear || freq < 0.25) {
			if (difference > 0.001) {
				writeSpectrum(spectrum, "fail-butterworth-spectrum");
				test.log(criticalDb, " != ", expectedDb);
				return test.fail("Butterworth critical frequency (lowpass)");
			}
		} else {
			// Slightly looser limits
			if (difference > 0.01) return test.fail("Butterworth critical frequency (lowpass)");
		}
		if (isBilinear) {
			if (std::abs(spectrum[nyquistIndex]) > zeroIsh) return test.fail("0 at Nyquist: ", spectrum[nyquistIndex]);
		}

		if (!isMonotonic(spectrum, -1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("lowpass not monotonic");
		}
	}

	{
		filter.highpass(freq, design);
		auto spectrum = getSpectrum(filter);
		int nyquistIndex = (int)spectrum.size()/2;
		
		// -3dB at critical point
		double criticalDb = ampToDb(interpSpectrum(spectrum, freq));
		double expectedDb = ampToDb(std::sqrt(0.5));
		double difference = std::abs(criticalDb - expectedDb);
		if (isBilinear || freq < 0.1) {
			if (difference > 0.001) return test.fail("Butterworth critical frequency (highpass)");
		} else {
			// Slightly looser limits
			if (difference > 0.01) return test.fail("Butterworth critical frequency (highpass)");
		}
		if (std::abs(spectrum[0]) > zeroIsh) return test.fail("0 at 0: ", spectrum[0]);
		if (isBilinear) {
			if (std::abs(spectrum[nyquistIndex] - one) > zeroIsh) return test.fail("1 at Nyquist: ", spectrum[nyquistIndex]);
		}

		if (!isMonotonic(spectrum, 1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("highpass not monotonic");
		}
	}

	// Wider bandwidth is just softer, still monotonic
	{
		filter.lowpass(freq, 1.91, design);
		auto spectrum = getSpectrum(filter);
		if (!isMonotonic(spectrum, -1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("lowpass octave=1.91 should still be monotonic");
		}
	}
	{
		filter.highpass(freq, 1.91, design);
		auto spectrum = getSpectrum(filter);
		if (!isMonotonic(spectrum, 1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("highpass octave=1.91 should still be monotonic");
		}
	}
	// Narrower bandwidth has a slight bump
	if (isBilinear || freq < 0.1) {
		filter.lowpass(freq, 1.89, design);
		auto spectrum = getSpectrum(filter);
		if (isMonotonic(spectrum, -1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("lowpass octave=1.89 should not be monotonic");
		}
	}
	if (isBilinear || freq < 0.01) {
		filter.highpass(freq, 1.89, design);
		auto spectrum = getSpectrum(filter);
		if (isMonotonic(spectrum, 1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("highpass octave=1.89 should not be monotonic");
		}
	}
}

TEST("Butterworth filters", filters_butterworth) {
	for (int i = 0; test.success && i < 10; ++i) {
		double f = test.random(0.02, 0.48);
		std::string n = std::to_string(f);
		testButterworth<double>(test.prefix("double@" + n), f);
		if (test.success) testButterworth<float>(test.prefix("float@" + n), f);
		
		signalsmith::filters::BiquadDesign design = signalsmith::filters::BiquadDesign::vicanek;
		if (test.success) testButterworth<double>(test.prefix("double-vicanek@" + n), f, design);
		if (test.success) testButterworth<float>(test.prefix("float-vicanek@" + n), f, design);
	}
}

