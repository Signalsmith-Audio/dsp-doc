// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "fft.h"
#include "../common.h"

#include "./filter-tests.h"

TEST("Filter design plots", filters_butterworth_plots) {
	auto drawShape = [&](int shape, std::string name) {
		Figure figure;
		int plotCounter = 0;
		bool isPeak = (shape == 3), isAllpass = (shape == 5);
		auto drawDesign = [&](signalsmith::filters::BiquadDesign design, std::string designName) {
			int plotColumn = plotCounter++;
			int plotRow = 0;
			auto &plot = figure(plotColumn, plotRow + 1).plot(200, (isPeak || isAllpass) ? 100 : 150);
			auto &plotFocus = (isPeak || isAllpass) ? plot : figure(plotColumn, plotRow).plot(200, 50);
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
				} else if (shape == 4) {
					filter.notch(freq, 1.66, design);
				} else {
					filter.allpass(freq, 1.66, design);
				}
				auto spectrum = getSpectrum(filter, 1e-10, 1024);

				auto &line = plot.line();
				auto &lineFocus = (isPeak || isAllpass) ? line : plotFocus.line();
				for (size_t i = 0; i < spectrum.size()/2; ++i) {
					double f = std::max(i*1.0/spectrum.size(), 1e-6);
					if (isAllpass) {
						auto phase = std::arg(spectrum[i]);
						if (phase > 0) phase -= 2*M_PI;
						line.add(f, phase);
					} else {
						line.add(f, ampToDb(std::abs(spectrum[i])));
						if (!isPeak) lineFocus.add(f, ampToDb(std::abs(spectrum[i])));
					}
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
			} else if (isAllpass) {
				plot.y.linear(-2*M_PI, 0).minor(0, "0").minor(-M_PI, u8"π").minor(-0.5*M_PI, "").minor(-2*M_PI, u8"-2π").minor(-1.5*M_PI, "");
			} else {
				plot.y.linear(-90, 1).minors(0, -12, -24, -48, -72);
			}
			plot.x.range(std::log, 0.001, 0.5).minors(0.001, 0.01, 0.1, 0.5)
				.minor(0.001*std::sqrt(10), "").minor(0.01*std::sqrt(10), "").minor(0.1*std::sqrt(10), "")
				.label("freq");
			if (!isPeak && !isAllpass) {
				plotFocus.y.linear(-6, 0).minor(-3, "").minors(0, -6);
				plotFocus.x.range(std::log, 0.001, 0.5).minor(0.001, "").minor(0.01, "").minor(0.1, "").minor(0.5, "")
					.minor(0.001*std::sqrt(10), "").minor(0.01*std::sqrt(10), "").minor(0.1*std::sqrt(10), "");
			}

			if (plotColumn == 0) {
				plot.y.label(isAllpass ? "phase" : "dB");
				plotFocus.y.label(isAllpass ? "phase" : "dB");
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
	drawShape(5, "allpass");
	return test.pass();
}
