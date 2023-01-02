#define EXPORT_NAME Main
#include "wasm-api.h"

#include "tests/plot.h"
#include "dsp/filters.h"

int main() {
}

#include <sstream>
#include <cmath>

extern "C" {
	EXPORT_API
	void filterGraph(double width, double height, int type, int intDesign, double freq, bool logFreq, double octaves, double db) {
		using Filter = signalsmith::filters::BiquadStatic<double>;
		double idealFactor = 0.01;
		Filter filter, idealFilter;

		Figure figure;
		auto &plot = figure.plot(width, height);
		plot.x.label("frequency");
		if (logFreq) {
			double low = 0.001;
			plot.x.range(std::log, low, 0.5).minors(0.001, 0.01, 0.1, 0.5);
		} else {
			plot.x.linear(0, 0.5).major(0).minors(0.25, 0.5);
		}
		plot.y.linear(-60, 24).label("dB").major(0).minors(24, 12, -12, -24, -36, -48, -60);
		
		auto addLine = [&](int intDesign, bool addIdeal) {
			signalsmith::filters::BiquadDesign design = (signalsmith::filters::BiquadDesign)intDesign;
			auto bilinear = signalsmith::filters::BiquadDesign::bilinear;
			if (type == 0) {
				filter.lowpass(freq, octaves, design);
				idealFilter.lowpass(freq*idealFactor, octaves, bilinear);
			} else if (type == 1) {
				filter.highpass(freq, octaves, design);
				idealFilter.highpass(freq*idealFactor, octaves, bilinear);
			} else if (type == 2) {
				filter.bandpass(freq, octaves, design);
				idealFilter.bandpass(freq*idealFactor, octaves, bilinear);
			} else if (type == 3) {
				filter.notch(freq, octaves, design);
				idealFilter.notch(freq*idealFactor, octaves, bilinear);
			} else if (type == 4) {
				filter.peakDb(freq, db, octaves, design);
				idealFilter.peakDb(freq*idealFactor, db, octaves, bilinear);
			} else if (type == 5) {
				filter.highShelfDb(freq, db, octaves, design);
				idealFilter.highShelfDb(freq*idealFactor, db, octaves, bilinear);
			} else if (type == 6) {
				filter.lowShelfDb(freq, db, octaves, design);
				idealFilter.lowShelfDb(freq*idealFactor, db, octaves, bilinear);
			} else {
			}

			auto &line = plot.line(), &idealLine = addIdeal ? plot.line() : line;
			for (double f = logFreq ? 0.001 : 0; f < 0.5; f += (f > 0.1 ? 1e-4 : f > 0.01 ? 1e-5 : 1e-6)) {
				line.add(f, std::max<double>(-300, filter.responseDb(f)));
				if (addIdeal) idealLine.add(f, std::max<double>(-300, idealFilter.responseDb(f*idealFactor)));
			}
			line.marker(freq, std::max<double>(-60, filter.responseDb(freq)));
		};
		if (intDesign >= 0) {
			addLine(intDesign, true);
		} else {
			addLine(0, true);
			addLine(1, false);
			addLine(2, false);
			addLine(3, false);
			plot.legend(0, 0)
				.add(0, "bilinear")
				.add(2, "cookbook")
				.add(3, "oneSided")
				.add(4, "vicanek")
				.add(1, "ideal");
		}
		
		std::stringstream stream;
		figure.write(stream);
		heapResultString(stream.str());
	}}
