// from the shared library
#include <test/tests.h>
#include "../common.h"

#include "filters.h"

#include <cmath>

TEST("Response matches") {
	using Sample = double;
	using Filter1 = signalsmith::filters::BiquadStatic<Sample>;
	using Filter2 = signalsmith::filters::Filter2<Sample>;
	
	signalsmith::plot::Figure figure;
	double osFactor = 100;
	Filter1 filter1, filter1os;
	Filter2 filter2;
	size_t plotIndex = 0;
	auto plotResponse = [&](size_t frames=1, std::function<double(size_t)> frameFunction=nullptr){
		auto &plot = figure(0, 2*(plotIndex++)).plot(500, 200);
		plot.x.linear(0, 0.5).major(0, "").minor(0.5, "");

		auto &phasePlot = figure(0, 2*(plotIndex++) + 1).plot(500, 100);
		phasePlot.x.copyFrom(plot.x);
		phasePlot.y.major(0).linear(-M_PI, M_PI).minor(-M_PI, u8"-π").minor(M_PI, u8"π");

		auto &line1os = plot.line();
		auto &line1 = plot.line();
		auto &line2 = plot.line();
		if (plotIndex == 1) {
			plot.legend(1, 1)
				.add(line1os, "ideal")
				.add(line1, "BiquadStatic")
				.add(line2, "Filter2");
		}

		auto &line1osP = phasePlot.line();
		auto &line1p = phasePlot.line();
		auto &line2p = phasePlot.line();

		double minDb = -3, maxDb = 3;

		for (size_t frame = 0; frame < frames; ++frame) {
			Sample frameTime = 0;
			if (frameFunction) frameTime = frameFunction(frame);
			for (Sample normFreq = 0; normFreq < 0.5; normFreq += Sample(0.001)) {
				auto db1os = filter1os.responseDb(normFreq/osFactor);
				auto db1 = filter1.responseDb(normFreq);
				auto db2 = filter2.responseDb(normFreq);
				
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
			plot.loopFrame(frameFunction(frames));
			phasePlot.loopFrame(frameFunction(frames));
		}

		minDb = std::max<Sample>(minDb, -60);
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

	filter1.lowpassQ(0.1, 2, signalsmith::filters::BiquadDesign::vicanek);
	filter1os.lowpassQ(0.1/osFactor, 2);
	filter2.lowpassQ(0.1, 2);
	plotResponse(100, [&](size_t frame){
		double p = 0.5 + 0.5*std::cos(frame*0.01*2*M_PI);
		double f = 0.0001 + 0.4998*p;
		double q = 2;
		filter1.lowpassQ(f, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.lowpassQ(f/osFactor, q);
		filter2.lowpassQ(f, q);
		return frame*0.03;
	});

	plotResponse(100, [&](size_t frame){
		double p = 0.5 + 0.5*std::cos(frame*0.01*2*M_PI);
		double f = 0.0001 + 0.4998*p;
		double q = 0.7;
		filter1.lowpassQ(f, q, signalsmith::filters::BiquadDesign::vicanek);
		filter1os.lowpassQ(f/osFactor, q);
		filter2.lowpassQ(f, q);
		return frame*0.03;
	});
	
	figure.write("filter2-response.svg");
	
	return test.pass();
}
