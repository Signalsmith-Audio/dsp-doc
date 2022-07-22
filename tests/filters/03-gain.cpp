// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "../common.h"

TEST("Gain", filters_gain) {
	signalsmith::filters::BiquadStatic<float> filter, filter2;
	float accuracy = 1e-4;

	for (int t = 0; t < 6; ++t) {
		if (t == 0) {
			filter.lowpass(test.random(0, 0.5), test.random(4, 12));
		} else if (t == 1) {
			filter.highpass(test.random(0, 0.5), test.random(4, 12));
		} else if (t == 2) {
			filter.notch(test.random(0, 0.5), test.random(4, 12));
		} else {
			filter.bandpass(test.random(0, 0.5), test.random(0.1, 10), test.random(4, 12));
		}

		double gain = test.random(0.1, 5);
		filter2 = filter;
		filter2.addGain(gain);
		
		for (int i = 0; i < 10; ++i) {
			float f = test.random(0, 0.5);
			TEST_APPROX(std::abs(filter.response(f))*gain, std::abs(filter2.response(f)), accuracy);
		}
	}
}

TEST("Gain", filters_gain_db) {
	signalsmith::filters::BiquadStatic<float> filter, filter2;
	float accuracy = 1e-4;

	for (int t = 0; t < 6; ++t) {
		if (t == 0) {
			filter.lowpass(test.random(0, 0.5), test.random(4, 12));
		} else if (t == 1) {
			filter.highpass(test.random(0, 0.5), test.random(4, 12));
		} else if (t == 2) {
			filter.notch(test.random(0, 0.5), test.random(4, 12));
		} else {
			filter.bandpass(test.random(0, 0.5), test.random(0.1, 10), test.random(4, 12));
		}

		double gain = test.random(-30, 30);
		filter2 = filter;
		filter2.addGainDb(gain);
		
		for (int i = 0; i < 10; ++i) {
			float f = test.random(0, 0.5);
			auto expected = filter.responseDb(f) + gain;
			auto actual = filter2.responseDb(f);
			if (expected > -60) {
				double a = accuracy;
				if (expected < -20) {
					a *= 10;
				}
				TEST_APPROX(expected, actual, a);
			}
		}
	}
}
