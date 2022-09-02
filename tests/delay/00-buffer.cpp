// from the shared library
#include <test/tests.h>

#include "delay.h"

#include "test-delay-stats.h"

TEST("Delay buffer stores data") {
	int delaySize = 100;
	signalsmith::delay::Buffer<double> buffer(delaySize);
	
	for (int i = 0; i < delaySize; ++i) {
		buffer[-i] = i;
	}
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(buffer[-i] == i);
	}
	
	++buffer;
	for (int i = 0; i < delaySize - 1; ++i) {
		// Incremented index
		TEST_ASSERT(buffer[-1 - i] == i);
	}
	--buffer;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(buffer[-i] == i);
	}
	auto view = buffer++;
	for (int i = 0; i < delaySize - 1; ++i) {
		TEST_ASSERT(buffer[-1 - i] == i);
		TEST_ASSERT(view[-i] == i);
	}
	view = buffer--;
	for (int i = 0; i < delaySize - 1; ++i) {
		TEST_ASSERT(view[-1 - i] == i);
		TEST_ASSERT(buffer[-i] == i);
	}

	buffer += 10;
	for (int i = 0; i < delaySize - 10; ++i) {
		TEST_ASSERT(buffer[-10 - i] == i);
	}
	buffer -= 20;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(buffer[10 - i] == i);
	}
	buffer += 10;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(buffer[-i] == i);
	}
	
	view = buffer + 10;
	for (int i = 0; i < delaySize - 10; ++i) {
		TEST_ASSERT(view[-10 - i] == i);
		TEST_ASSERT(buffer[-i] == i);
	}
	view = view - 20;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(view[10 - i] == i);
		TEST_ASSERT(buffer[-i] == i);
	}
	view = buffer - 10;
	for (int i = 0; i < delaySize; ++i) {
		TEST_ASSERT(view[10 - i] == i);
		TEST_ASSERT(buffer[-i] == i);
	}
	view = buffer.view(5);
	for (int i = 0; i < delaySize - 5; ++i) {
		TEST_ASSERT(view[-5 - i] == i);
		TEST_ASSERT(buffer[-i] == i);
	}
}
