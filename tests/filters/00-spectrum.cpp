// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "fft.h"
#include "../common.h"

#include <vector>
#include <complex>

using Spectrum = std::vector<std::complex<double>>;

double ampToDb(double amp) {
	return 20*std::log10(std::max(amp, 1e-100));
}

template<class Filter>
Spectrum getSpectrum(Filter &filter, int impulseSize=8192) {
	filter.reset();
	Spectrum impulse(impulseSize), spectrum(impulseSize);

	for (int i = 0; i < impulseSize; ++i) {
		int v = (i == 0) ? 1 : 0;
		impulse[i] = filter(v);
	}
	
	signalsmith::FFT<double> fft(impulseSize);
	fft.fft(impulse, spectrum);
	return spectrum;
}

double interpSpectrum(Spectrum spectrum, double freq) {
	double index = freq*spectrum.size();
	int intFreqLow = index;
	double ratio = index - intFreqLow;
	int intFreqHigh = intFreqLow + 1;
	if (intFreqHigh >= (int)spectrum.size()) {
		--intFreqLow;
		--intFreqHigh;
	}
	double energyLow = std::norm(spectrum[intFreqLow]);
	double energyHigh = std::norm(spectrum[intFreqHigh]);
	return std::sqrt(energyLow + (energyHigh - energyLow)*ratio);
}

template<class Filter>
int impulseLength(Filter &filter, double limit=1e-10) {
	filter.reset();
	filter(1);

	// Count how long the impulse response takes to go below the limit
	int sample = 0;
	int belowLimitCounter = 0;
	while (sample < 100 || belowLimitCounter < sample*0.1 /*overshoot by 10% to be sure*/) {
		auto mag = std::abs(filter(0));
		if (mag < limit) {
			++belowLimitCounter;
		} else {
			belowLimitCounter = 0;
		}
		++sample;
	}
	
	return sample;
}

void writeSpectrum(Spectrum spectrum, std::string name) {
	CsvWriter csv(name);
	csv.line("freq", "dB", "phase", "group delay");

	double prevPhase = 0, prevMag = 0;
	for (size_t i = 0; i <= spectrum.size()/2; ++i) {
		auto bin = spectrum[i];
		double mag = std::abs(bin);
		double db = 20*std::log10(mag);
		double phase = std::arg(bin);
		double phaseDiff = (prevPhase - phase);
		if (mag < 1e-10 || prevMag < 1e-10) {
			phaseDiff = 0;
		} else if (phaseDiff > M_PI) {
			phaseDiff -= 2*M_PI;
		} else if (phaseDiff <= -M_PI) {
			phaseDiff += 2*M_PI;
		}
		double groupDelay = phaseDiff*spectrum.size()/(2*M_PI);
		prevMag = mag;
		prevPhase = phase;
		csv.line(i*1.0/spectrum.size(), db, phase, groupDelay);
	}
}

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
		if (mag < maxMag) {
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
	
	double zeroIsh = 1e-6; // -120dB
	
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
			return test.fail("Butterworth critical frequency (lowpass)");
		}
		if (std::abs(spectrum[nyquistIndex]) > zeroIsh) return test.fail("0 at Nyquist: ", spectrum[nyquistIndex]);
		if (std::abs(std::abs(spectrum[0]) - 1) > zeroIsh) return test.fail("1 at 0: ", spectrum[0]);

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
		if (std::abs(std::abs(spectrum[nyquistIndex]) - 1) > zeroIsh) return test.fail("1 at Nyquist: ", spectrum[nyquistIndex]);

		if (!isMonotonic(spectrum, 1)) {
			writeSpectrum(spectrum, "fail-butterworth-spectrum");
			return test.fail("highpass not monotonic");
		}
	}

	// Non-Butterworth
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
	testButterworth<double>(test.prefix("double@0.01"), 0.1);
	testButterworth<float>(test.prefix("float@0.01"), 0.1);
}
