// from the shared library
#include <test/tests.h>

#include "../common.h"
#include "dsp/mix.h"

#include <array>
#include <cmath>

TEST("Cheap cross-fade", crossfade) {
	float to, from;
	signalsmith::mix::cheapEnergyCrossfade(0.0, to, from);
	TEST_APPROX(to, 0, 1e-6);
	TEST_APPROX(from, 1, 1e-6);
	signalsmith::mix::cheapEnergyCrossfade(1, to, from);
	TEST_APPROX(to, 1, 1e-6);
	TEST_APPROX(from, 0, 1e-6);
	signalsmith::mix::cheapEnergyCrossfade(0.5, to, from);
	TEST_APPROX(to, from, 1e-5);
	TEST_APPROX(to, std::sqrt(0.5), 0.01);
	TEST_APPROX(from, std::sqrt(0.5), 0.01);
	
	for (double x = 0; x < 1; x += 0.01) {
		float to, from;
		signalsmith::mix::cheapEnergyCrossfade(x, to, from);
		double energy = to*to + from*from;
		TEST_APPROX(energy, 1, 0.011 /* 1.1% error */);
		
		TEST_ASSERT(to > -0.00001);
		TEST_ASSERT(from > -0.00001);
		TEST_ASSERT(to < 1.004);
		TEST_ASSERT(from < 1.004);
	}
}
