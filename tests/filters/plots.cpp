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
		bool isPeak = (shape == 3), isAllpass = (shape == 5), isShelf = (shape == 6 || shape == 7);
		bool singlePlot = (isPeak || isAllpass || isShelf);
		auto drawDesign = [&](signalsmith::filters::BiquadDesign design, std::string designName) {
			int plotColumn = plotCounter++;
			int plotRow = 0;
			auto &plot = figure(plotColumn, plotRow + 1).plot(200, singlePlot ? 100 : 150);
			auto &plotFocus = singlePlot ? plot : figure(plotColumn, plotRow).plot(200, 50);
			auto drawLine = [&](double freq) {
				signalsmith::filters::BiquadStatic<double> filter;
				if (shape == 0) {
					filter.lowpass(freq, design);
				} else if (shape == 1) {
					filter.highpass(freq, design);
				} else if (shape == 2) {
					filter.bandpass(freq, 1.66, design);
				} else if (shape == 3) {
					filter.peak(freq, 4, 1.66, design);
				} else if (shape == 4) {
					filter.notch(freq, 1.66, design);
				} else if (shape == 5) {
					filter.allpass(freq, 1.66, design);
				} else if (shape == 6) {
					filter.highShelfDb(freq, 12, filter.defaultBandwidth, design);
				} else {
					filter.lowShelfDbQ(freq, 12, filter.defaultQ, design);
				}

				auto &line = plot.line();
				auto &lineFocus = singlePlot ? line : plotFocus.line();
				int freqCount = 16384;
				for (int fi = 1; fi < freqCount; ++fi) {
					double f = fi*0.5/(freqCount - 1);
					auto response = filter.response(f);
					if (isAllpass) {
						auto phase = std::arg(response);
						if (phase > 0) phase -= 2*M_PI;
						line.add(f, phase);
					} else {
						line.add(f, ampToDb(std::abs(response)));
						if (!singlePlot) lineFocus.add(f, ampToDb(std::abs(response)));
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
			if (isPeak || isShelf) {
				plot.y.linear(-2, 14).minors(0, 6, 12);
			} else if (isAllpass) {
				plot.y.linear(-2*M_PI, 0).minor(0, "0").minor(-M_PI, u8"π").minor(-0.5*M_PI, "").minor(-2*M_PI, u8"-2π").minor(-1.5*M_PI, "");
			} else {
				plot.y.linear(-90, 1).minors(0, -12, -24, -48, -72);
			}
			plot.x.range(std::log, 0.001, 0.5).minors(0.001, 0.01, 0.1, 0.5)
				.minor(0.001*std::sqrt(10), "").minor(0.01*std::sqrt(10), "").minor(0.1*std::sqrt(10), "")
				.label("freq");
			if (!singlePlot) {
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
	drawShape(6, "high-shelf");
	drawShape(7, "low-shelf");
	return test.pass();
}
