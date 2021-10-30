// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "fft.h"
#include "../common.h"

#include "../filter-tests.h"

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

bool isMonotonic(const Spectrum &spectrum, int direction=-1) {
	// Monotonically increasing in the specified direction
	int start = (direction > 0) ? 0 : spectrum.size()/2;
	int end = (direction > 0) ? spectrum.size()/2 + 1 : -1;

	double maxMag = std::abs(spectrum[start]);
	for (int i = start; i != end; i += direction) {
		double mag = std::abs(spectrum[i]);
		if (maxMag > 1e-6 && mag < maxMag) { // only care if it's above -120dB
			//std::cout << "not monotonic @" << i << " (" << i*1.0/spectrum.size() << ")\n";
			return false;
		}
		maxMag = std::max(mag, maxMag);
	}
	return true;
}

// Should be Butterworth when we don't specify a bandwidth
template<typename Sample>
void testButterworth(Test &&test, double freq) {
	signalsmith::filters::BiquadStatic<Sample> filter;
	
	double zeroIsh = 1e-5; // -100dB
	std::complex<double> one = 1;
	
	{
		filter.lowpass(freq);
		auto spectrum = getSpectrum(filter);
		int nyquistIndex = (int)spectrum.size()/2;
		
		// -3dB at critical point
		double criticalDb = ampToDb(interpSpectrum(spectrum, freq));
		double expectedDb = ampToDb(std::sqrt(0.5));
		double difference = std::abs(criticalDb - expectedDb);
		if (difference > 0.001) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			test.log(criticalDb, " != ", expectedDb);
			return test.fail("Butterworth critical frequency (lowpass)");
		}
		if (std::abs(spectrum[nyquistIndex]) > zeroIsh) return test.fail("0 at Nyquist: ", spectrum[nyquistIndex]);
		if (std::abs(spectrum[0] - one) > zeroIsh) return test.fail("1 at 0: ", spectrum[0]);

		if (!isMonotonic(spectrum, -1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("lowpass not monotonic");
		}
	}

	{
		filter.highpass(freq);
		auto spectrum = getSpectrum(filter);
		int nyquistIndex = (int)spectrum.size()/2;
		
		// -3dB at critical point
		double criticalDb = ampToDb(interpSpectrum(spectrum, freq));
		double expectedDb = ampToDb(std::sqrt(0.5));
		double difference = std::abs(criticalDb - expectedDb);
		if (difference > 0.001) return test.fail("Butterworth critical frequency (highpass)");
		if (std::abs(spectrum[0]) > zeroIsh) return test.fail("0 at 0: ", spectrum[0]);
		if (std::abs(spectrum[nyquistIndex] - one) > zeroIsh) return test.fail("1 at Nyquist: ", spectrum[nyquistIndex]);

		if (!isMonotonic(spectrum, 1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("highpass not monotonic");
		}
	}

	// Wider bandwidth is just softer, still monotonic
	{
		filter.lowpass(freq, 1.91);
		auto spectrum = getSpectrum(filter);
		if (!isMonotonic(spectrum, -1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("lowpass octave=1.91 should still be monotonic");
		}
	}
	{
		filter.highpass(freq, 1.91);
		auto spectrum = getSpectrum(filter);
		if (!isMonotonic(spectrum, 1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("highpass octave=1.91 should still be monotonic");
		}
	}
	// Narrower bandwidth has a slight bump
	{
		filter.lowpass(freq, 1.89);
		auto spectrum = getSpectrum(filter);
		if (isMonotonic(spectrum, -1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("lowpass octave=1.89 should not be monotonic");
		}
	}
	{
		filter.highpass(freq, 1.89);
		auto spectrum = getSpectrum(filter);
		if (isMonotonic(spectrum, 1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("highpass octave=1.89 should not be monotonic");
		}
	}
}

TEST("Butterworth filters", filters_butterworth) {
	testButterworth<double>(test.prefix("double@0.05"), 0.05);
	if (test.success) testButterworth<float>(test.prefix("float@0.05"), 0.05);
	if (test.success) testButterworth<double>(test.prefix("double@0.2"), 0.2);
	if (test.success) testButterworth<float>(test.prefix("float@0.2"), 0.2);
}
