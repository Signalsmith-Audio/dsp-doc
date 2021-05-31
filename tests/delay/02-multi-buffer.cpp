// from the shared library
#include <test/tests.h>

#include "delay.h"

#include "test-delay-stats.h"

TEST("Multi-channel buffer stores data", delay_interp_none) {
	int delaySize = 100;
	int channels = 4;
	using MultiBuffer = signalsmith::delay::MultiBuffer<double>;
	MultiBuffer multiBuffer(channels, delaySize);
	
	for (int c = 0; c < channels; ++c) {
		for (int i = 0; i < delaySize; ++i) {
			multiBuffer[c][-i] = i + c*delaySize;
		}
	}
	const MultiBuffer &constMultiBuffer = multiBuffer;
	for (int c = 0; c < channels; ++c) {
		for (int i = 0; i < delaySize; ++i) {
			TEST_ASSERT(multiBuffer[c][-i] == i + c*delaySize);
			TEST_ASSERT(constMultiBuffer[c][-i] == i + c*delaySize);
		}
	}

	for (int i = 0; i < delaySize; ++i) {
		auto value = multiBuffer.at(-i);
		auto constValue = constMultiBuffer.at(-i);
		
		std::vector<double> expected(channels), zeros(channels, 0);
		std::vector<double> read(channels);
		constMultiBuffer.at(-i).get(read);
		for (int c = 0; c < channels; ++c) {
			expected[c] = i + c*delaySize;
			TEST_ASSERT(value[c] == expected[c]);
			TEST_ASSERT(constValue[c] == expected[c]);
			TEST_ASSERT(read[c] == expected[c]);
			
			value[c] = 0;
			TEST_ASSERT(value[c] == 0);
			TEST_ASSERT(read[c] == expected[c]);
		}
		// Reset value
		value = expected;
		for (int c = 0; c < channels; ++c) {
			TEST_ASSERT(value[c] == expected[c]);
		}
		multiBuffer.at(-i) = zeros;
		for (int c = 0; c < channels; ++c) {
			TEST_ASSERT(value[c] == 0);
		}
		multiBuffer.at(-i)[0] = 5.5;
		TEST_ASSERT(multiBuffer[0][-i] == 5.5);
		multiBuffer.at(-i).set(expected);
		for (int c = 0; c < channels; ++c) {
			TEST_ASSERT(value[c] == expected[c]);
		}
	}

	// Tests adapted from the main delayBuffer stuff
	
	++multiBuffer;
	for (int i = 0; i < delaySize - 1; ++i) {
		// Incremented index
		TEST_ASSERT(multiBuffer[0][-1 - i] == i);
	}
	--multiBuffer;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(multiBuffer[0][-i] == i);
	}
	auto view = (multiBuffer++)[0];
	for (int i = 0; i < delaySize - 1; ++i) {
		TEST_ASSERT(multiBuffer[0][-1 - i] == i);
		TEST_ASSERT(view[-i] == i);
	}
	view = (multiBuffer--)[0];
	for (int i = 0; i < delaySize - 1; ++i) {
		TEST_ASSERT(view[-1 - i] == i);
		TEST_ASSERT(multiBuffer[0][-i] == i);
	}

	multiBuffer += 10;
	for (int i = 0; i < delaySize - 10; ++i) {
		TEST_ASSERT(multiBuffer[0][-10 - i] == i);
	}
	multiBuffer -= 20;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(multiBuffer[0][10 - i] == i);
	}
	multiBuffer += 10;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(multiBuffer[0][-i] == i);
	}
	
	view = (multiBuffer + 10)[0];
	for (int i = 0; i < delaySize - 10; ++i) {
		TEST_ASSERT(view[-10 - i] == i);
		TEST_ASSERT(multiBuffer[0][-i] == i);
	}
	view = view - 20;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(view[10 - i] == i);
		TEST_ASSERT(multiBuffer[0][-i] == i);
	}
	view = (multiBuffer - 10)[0];
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(view[10 - i] == i);
		TEST_ASSERT(multiBuffer[0][-i] == i);
	}
	view = multiBuffer.view(5)[0];
	for (int i = 0; i < delaySize - 5; ++i) {
		TEST_ASSERT(view[-5 - i] == i);
		TEST_ASSERT(multiBuffer[0][-i] == i);
	}
	view = multiBuffer.view(3)[0] + 5;
	for (int i = 0; i < delaySize - 8; ++i) {
		TEST_ASSERT(view[-8 - i] == i);
		TEST_ASSERT(multiBuffer[0][-i] == i);
	}
}
