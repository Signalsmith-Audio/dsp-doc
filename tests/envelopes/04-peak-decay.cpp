#include <test/tests.h>
#include "../common.h"

#include <array>
#include <cmath>

#include "envelopes.h"

TEST("Peak decay linear (example)") {
	int length = 250;
	signalsmith::envelopes::CubicLfo lfo(12345);
	lfo.set(0, 1, 0.05, 2, 1);

	signalsmith::envelopes::PeakDecayLinear<float> decayA(20), decayB(40);
	
	CsvWriter csv("peak-decay-linear");
	csv.line("i", "signal", "decay (20)", "decay (40)");
	for (int i = 0; i < length; ++i) {
		double v = lfo.next();
		v = std::max<double>(0, 2*v - 1);
		v = v*v*10;
		if (i < 90) v += 3;
		if (i >= 90 && i < 100) {
			v += 3*(1 - (i - 90)*0.1);
		}
		csv.line(i, v, decayA(v), decayB(v));
	}
	return test.pass();
}

TEST("Peak decay linear cascade (example)") {
	int length = 250;
	signalsmith::envelopes::CubicLfo lfo(12345);
	lfo.set(0, 1, 0.05, 2, 1);

	signalsmith::envelopes::PeakDecayLinear<float> decayA(40), decayB(30), decayC(10), decayD(10), decayE(30);
	
	CsvWriter csv("peak-decay-linear-cascade");
	csv.line("i", "signal", "decay (40)", "decay (30-10)", "decay (10-30)");
	for (int i = 0; i < length; ++i) {
		double v = lfo.next();
		v = std::max<double>(0, 2*v - 1);
		v = v*v*10;
		if (i < 90) v += 3;
		if (i >= 90 && i < 100) {
			v += 3*(1 - (i - 90)*0.1);
		}
		csv.line(i, v, decayA(v), decayC(decayB(v)), decayE(decayD(v)));
	}
	return test.pass();
}

