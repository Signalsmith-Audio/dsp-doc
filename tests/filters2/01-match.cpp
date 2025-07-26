// from the shared library
#include <test/tests.h>
#include "../common.h"

#include "filters.h"

#include <cmath>

TEST("Response matches") {
	using Sample = double;
	using Filter1 = signalsmith::filters::BiquadStatic<double>;
	using Filter2 = signalsmith::filters::Filter2<Sample>;
	
	signalsmith::plot::Figure figure;
	double osFactor = 16;
	Filter1 filter1, filter1os;
	Filter2 filter2;
	size_t plotIndex = 0;
	
	auto toDb = [](std::complex<Sample> response){
		return 10*std::log10(std::norm(response) + Sample(1e-30));
	};
	struct Lines {
		signalsmith::plot::Line2D &markers, &markersP, &line1os, &line1, &line2, &line1osP, &line1p, &line2p;
	};
	auto plotResponse = [&](const char *name, size_t frames=1, std::function<double(size_t, Lines)> frameFunction=nullptr){
		auto plotCol = plotIndex/2;
		auto plotRow = plotIndex%2;
		++plotIndex;
		auto &plot = figure(plotCol, plotRow*2).plot(300, 150);
		auto &phasePlot = figure(plotCol, plotRow*2 + 1).plot(300, 80);
		plot.title(name);

		auto &line1os = plot.line();
		auto &line1 = plot.line();
		auto &line2 = plot.line();
		if (plotIndex == 1) {
			plot.legend(0, 0)
				.add(line1os, "ideal")
				.add(line1, "BiquadStatic")
				.add(line2, "Filter2");
		}

		auto &line1osP = phasePlot.line();
		auto &line1p = phasePlot.line();
		auto &line2p = phasePlot.line();
		Lines lines{plot.line(-1), phasePlot.line(-1), line1os, line1, line2, line1osP, line1p, line2p};

		double minDb = -3, maxDb = 3;

		for (size_t frame = 0; frame < frames; ++frame) {
			Sample frameTime = 0;
			if (frameFunction) frameTime = frameFunction(frame, lines);
			for (Sample normFreq = 1e-10; normFreq < 0.5; normFreq *= 1.02) {
				auto db1os = toDb(filter1os.response(normFreq/osFactor));
				auto db1 = toDb(filter1.response(normFreq));
				auto db2 = toDb(filter2.response(normFreq));
				
				line1os.add(normFreq, db1os);
				line1.add(normFreq, db1);
				line2.add(normFreq, db2);
				minDb = std::min(db1os, minDb);
				minDb = std::min(db2, minDb);
				maxDb = std::max(db1os, maxDb);
				
				line1osP.add(normFreq, std::arg(filter1os.response(normFreq/osFactor)));
				line1p.add(normFreq, std::arg(filter1.response(normFreq)));
				line2p.add(normFreq, std::arg(filter2.response(normFreq)));
			}
			if (frameFunction) {
				plot.toFrame(frameTime);
				phasePlot.toFrame(frameTime);
			}
		}
		if (frameFunction) {
			auto loopTime = frameFunction(frames, lines);
			plot.loopFrame(loopTime);
			phasePlot.loopFrame(loopTime);
		}

		// plot limits
		int maxDiv = 1<<7;
		plot.x.range(std::log, 1.0/maxDiv, 0.5).major(1.0/maxDiv, "1/" + std::to_string(maxDiv));
		bool alternator = true;
		for (int d = 2; d < maxDiv; d *= 2) {
			plot.x.minor(1.0/d, alternator ? "1/" + std::to_string(d) : "");
			alternator = !alternator;
		}
		phasePlot.x.copyFrom(plot.x);
		phasePlot.y.major(0).linear(-M_PI, M_PI).minor(-M_PI, u8"-π").minor(M_PI, u8"π");

		minDb = std::max<Sample>(minDb, -50);
		plot.y.linear(minDb - 1, maxDb + 1);
		
		double rangeDb = maxDb - minDb;
		int dbStep = (rangeDb < 12) ? 3 : (rangeDb < 35) ? 6 : (rangeDb < 80) ? 10 : 20;
		plot.y.major(0);
		for (int db = dbStep; db <= maxDb + 1; db += dbStep) {
			plot.y.minor(db);
		}
		for (int db = -dbStep; db >= minDb - 1; db -= dbStep) {
			plot.y.minor(db);
		}
	};

	size_t frameCount = 500;
	auto funcF = [&](size_t frame){
		double p = frame*2*M_PI/frameCount;
		return 0.51*std::pow(std::abs(std::sin(p/2)), 4);
	};
	auto funcQ = [&](size_t frame){
		double p = frame*2*M_PI/frameCount;
		return 2 + 1.8*std::sin(p*6);
	};
	auto funcG = [&](size_t frame){
		double p = frame*2*M_PI/frameCount;
		return std::exp2(2*std::sin(p*4));
	};
	plotResponse("lowpass", frameCount, [&](size_t frame, Lines lines){
		double f = funcF(frame), q = funcQ(frame);
		filter1.lowpassQ(f, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.lowpassQ(f/osFactor, q);
		filter2.lowpassQ(f, q);

		lines.markers.marker(f, 20*std::log10(q));
		lines.markersP.marker(f, -M_PI/2, -1);
		if (f > filter2.maxRefFreq) {
			lines.markers.marker(filter2.maxRefFreq, toDb(filter1os.response(filter2.maxRefFreq/osFactor)));
			lines.markersP.marker(filter2.maxRefFreq, std::arg(filter1os.response(filter2.maxRefFreq/osFactor)));
		}
		return frame*0.05;
	});

	plotResponse("highpass", frameCount, [&](size_t frame, Lines lines){
		double f = funcF(frame), q = funcQ(frame);
		filter1.highpassQ(f, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.highpassQ(f/osFactor, q);
		filter2.highpassQ(f, q);

		lines.markers.marker(f, 20*std::log10(q), -1);
		lines.markersP.marker(f, 0, -1);
		if (f > filter2.maxRefFreq) {
			lines.markers.marker(filter2.maxRefFreq, toDb(filter1os.response(filter2.maxRefFreq/osFactor)));
		}
		return frame*0.05;
	});

	plotResponse("bandpass", frameCount, [&](size_t frame, Lines lines){
		double f = funcF(frame), q = funcQ(frame);
		filter1.bandpassQ(f, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.bandpassQ(f/osFactor, q);
		filter2.bandpassQ(f, q);

		lines.markers.marker(f, 0, -1);
		lines.markersP.marker(f, 0, -1);
		if (f > filter2.maxRefFreq) {
			lines.markers.marker(filter2.maxRefFreq, toDb(filter1os.response(filter2.maxRefFreq/osFactor)));
		}
		return frame*0.05;
	});

	plotResponse("notch", frameCount, [&](size_t frame, Lines lines){
		double f = funcF(frame), q = funcQ(frame);
		filter1.notchQ(f, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.notchQ(f/osFactor, q);
		filter2.notchQ(f, q);

		lines.markers.marker(f, 0, -1);
		lines.markersP.marker(f, 0, -1);
		if (f > filter2.maxRefFreq) {
			lines.markers.marker(filter2.maxRefFreq, toDb(filter1os.response(filter2.maxRefFreq/osFactor)));
		}
		return frame*0.05;
	});

	plotResponse("peak", frameCount, [&](size_t frame, Lines lines){
		double f = funcF(frame), q = funcQ(frame);
		double gain = funcG(frame);
		filter1.peakQ(f, gain, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.peakQ(f/osFactor, gain, q);
		filter2.peakQ(f, gain, q);

		lines.markers.marker(f, 20*std::log10(gain), -1);
		lines.markersP.marker(f, 0, -1);
		if (f > filter2.maxRefFreq) {
			lines.markers.marker(filter2.maxRefFreq, toDb(filter1os.response(filter2.maxRefFreq/osFactor)));
		}
		return frame*0.05;
	});

	plotResponse("allpass", frameCount, [&](size_t frame, Lines lines){
		double f = funcF(frame), q = funcQ(frame);
		filter1.allpassQ(f, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.allpassQ(f/osFactor, q);
		filter2.allpassQ(f, q);

		lines.markers.marker(f, 0, -1);
		lines.markersP.marker(f, 0, -1);
		if (f > filter2.maxRefFreq) {
			lines.markers.marker(filter2.maxRefFreq, toDb(filter1os.response(filter2.maxRefFreq/osFactor)));
		}
		return frame*0.05;
	});

	plotResponse("high-shelf", frameCount, [&](size_t frame, Lines lines){
		double f = funcF(frame), q = funcQ(frame);
		double gain = funcG(frame);
		filter1.highShelfQ(f, gain, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.highShelfQ(f/osFactor, gain, q);
		filter2.highShelfQ(f, gain, q);

		lines.markers.marker(f, 20*std::log10(std::sqrt(gain)), -1);
		lines.markersP.marker(f, 0, -1);
		if (f > filter2.maxRefFreq) {
			lines.markers.marker(filter2.maxRefFreq, toDb(filter1os.response(filter2.maxRefFreq/osFactor)));
		}
		return frame*0.05;
	});

	plotResponse("low-shelf", frameCount, [&](size_t frame, Lines lines){
		double f = funcF(frame), q = funcQ(frame);
		double gain = funcG(frame);
		filter1.lowShelfQ(f, gain, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.lowShelfQ(f/osFactor, gain, q);
		filter2.lowShelfQ(f, gain, q);

		lines.markers.marker(f, 20*std::log10(std::sqrt(gain)), -1);
		lines.markersP.marker(f, 0, -1);
		if (f > filter2.maxRefFreq) {
			lines.markers.marker(filter2.maxRefFreq, toDb(filter1os.response(filter2.maxRefFreq/osFactor)));
		}
		return frame*0.05;
	});
	
	figure.write("filter2-response.svg");
	
	return test.pass();
}
