#include <test/tests.h>
#include "../common.h"

#include <array>
#include <cmath>

#include "envelopes.h"

TEST("Linear decay (example)", linear_decay_example) {
	int length = 250;
	signalsmith::envelopes::CubicLfo lfo(12345);
	lfo.set(0, 1, 0.05, 2, 1);

	signalsmith::envelopes::PeakDecayLinear<float> decayA(20), decayB(50), decayC(10), decayD(10);

	CsvWriter csv("linear-decay");
	csv.line("i", "signal", "decay (20)", "decay (50)", "decay (10) x2");
	for (int i = 0; i < length; ++i) {
		double v = lfo.next();
		v = std::max<double>(0, 2*v - 1);
		v = v*v*10;
		csv.line(i, v, decayA(v), decayB(v), decayD(decayC(v)));
	}
	return test.pass();
}

