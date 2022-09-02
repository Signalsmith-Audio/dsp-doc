// from the shared library
#include <test/tests.h>

#include "delay.h"
#include "test-delay-stats.h"

#include <iostream>
#include <array>

TEST("Multi-Delay") {
	constexpr int channels = 3;
	int delayLength = 80;
	
	signalsmith::delay::MultiDelay<double> multiDelay(channels, delayLength);
	
	// Put a known sequence in
	for (int i = 0; i < delayLength; ++i) {
		std::array<double, channels> values;
		for (int c = 0; c < channels; ++c) values[c] = c + i*channels;
		multiDelay.write(values);
	}

	// Read out delayed samples
	for (int i = 0; i < delayLength; ++i) {
		std::array<double, channels> zero;
		for (int c = 0; c < channels; ++c) zero[c] = 0;

		auto values = multiDelay.write(zero).read(delayLength);
		for (int c = 0; c < channels; ++c) {
			TEST_ASSERT(values[c] == c + i*channels);
		}
	}
}
