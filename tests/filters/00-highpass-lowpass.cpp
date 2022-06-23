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

TEST("Butterworth plots", filters_butterworth_plots) {
	auto drawShape = [&](int shape, std::string name) {
		Figure figure;
		int plotCounter = 0;
		bool isPeak = (shape == 3);
		auto drawDesign = [&](signalsmith::filters::BiquadDesign design, std::string designName) {
			int plotColumn = plotCounter++;
			int plotRow = 0;
			auto &plot = figure(plotColumn, plotRow + 1).plot(200, isPeak ? 100 : 150);
			auto &plotFocus = isPeak ? plot : figure(plotColumn, plotRow).plot(200, 50);
			auto drawLine = [&](double freq) {
				signalsmith::filters::BiquadStatic<double> filter;
				if (shape == 0) {
					filter.lowpass(freq, design);
				} else if (shape == 1) {
					filter.highpass(freq, 1.8, design);
				} else if (shape == 2) {
					filter.bandpass(freq, 1.66, design);
				} else if (shape == 3) {
					filter.peak(freq, 4, 1.66, design);
				} else {
					filter.notch(freq, 1.66, design);
				}
				auto spectrum = getSpectrum(filter, 1e-10, 1024);

				auto &line = plot.line();
				auto &lineFocus = isPeak ? line : plotFocus.line();
				for (size_t i = 0; i < spectrum.size()/2; ++i) {
					double f = std::max(i*1.0/spectrum.size(), 1e-6);
					line.add(f, ampToDb(std::abs(spectrum[i])));
					if (!isPeak) lineFocus.add(f, ampToDb(std::abs(spectrum[i])));
				}
			};
			drawLine(0.001*std::sqrt(10));
			drawLine(0.01);
			drawLine(0.01*std::sqrt(10));
			drawLine(0.1);
			drawLine(0.1*std::sqrt(10));
			drawLine(0.49);

			// Draw top label as title
			plotFocus.newX().flip().label(designName);
			if (isPeak) {
				plot.y.linear(-2, 14).minors(0, 6, 12);
			} else {
				plot.y.linear(-90, 1).minors(0, -12, -24, -48, -72);
			}
			plot.x.range(std::log, 0.001, 0.5).minors(0.001, 0.01, 0.1, 0.5)
				.minor(0.001*std::sqrt(10), "").minor(0.01*std::sqrt(10), "").minor(0.1*std::sqrt(10), "")
				.label("freq");
			if (!isPeak) {
				plotFocus.y.linear(-6, 0).minor(-3, "").minors(0, -6);
				plotFocus.x.range(std::log, 0.001, 0.5).minor(0.001, "").minor(0.01, "").minor(0.1, "").minor(0.5, "")
					.minor(0.001*std::sqrt(10), "").minor(0.01*std::sqrt(10), "").minor(0.1*std::sqrt(10), "");
			}

			if (plotColumn == 0) {
				plot.y.label("dB");
				plotFocus.y.label("dB");
			} else {
				plot.y.blankLabels();
				plotFocus.y.blankLabels();
			}
		};
		drawDesign(signalsmith::filters::BiquadDesign::bilinear, "bilinear");
		drawDesign(signalsmith::filters::BiquadDesign::cookbook, "cookbook");
		drawDesign(signalsmith::filters::BiquadDesign::oneSided, "oneSided");
		drawDesign(signalsmith::filters::BiquadDesign::vicanek, "vicanek");
		
		figure.write("filters-" + name + ".svg");
	};
	drawShape(0, "lowpass");
	drawShape(1, "highpass");
	drawShape(2, "bandpass");
	drawShape(3, "peak");
	drawShape(4, "notch");
	return test.pass();
}
