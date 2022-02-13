#include <test/tests.h>
#include "../common.h"

#include <array>
#include <cmath>

#include "envelopes.h"

TEST("Peak hold", peak_hold) {
	int length = 1000;
	std::vector<float> signal(length);
	for (auto &v : signal) {
		v = test.random(-1, 1);
	}
	
	int maxLength = 100, holdLength = 50;
	float startingPeak = -5;
	signalsmith::envelopes::PeakHold<float> peakHold(maxLength);
	TEST_ASSERT(peakHold.size() == maxLength);
	peakHold.reset(5);
	TEST_ASSERT(peakHold.read() == 5);
	TEST_ASSERT(peakHold.size() == maxLength);
	peakHold.set(holdLength);
	TEST_ASSERT(peakHold.size() == holdLength);
	peakHold.reset(startingPeak);
	TEST_ASSERT(peakHold.size() == holdLength);

	for (int i = 0; i < length; ++i) {
		float result = peakHold(signal[i]);

		int start = std::max(0, i - holdLength + 1);
		float peak = (i >= holdLength ? signal[i] : startingPeak);
		for (int j = start; j <= i; ++j) {
			peak = std::max(peak, signal[j]);
		}
		TEST_ASSERT(result == peak);
	}
	
	// Test all sizes
	for (int size = 0; size <= maxLength; ++size) {
		peakHold.reset();
		peakHold.set(size);
		for (int i = 0; i < length; ++i) {
			float actual = peakHold(signal[i]);
			TEST_ASSERT(actual == peakHold.read());
			if (i >= size - 1) {
				float peak = std::numeric_limits<float>::lowest();
				for (int j = i + 1 - size; j <= i; ++j) {
					peak = std::max(peak, signal[j]);
				}
				TEST_ASSERT(peakHold.read() == peak);
			}
		}
	}
}

TEST("Peak hold (example)", peak_hold_example) {
	int length = 250;
	signalsmith::envelopes::CubicLfo lfo(1248);
	lfo.set(0, 10, 0.05, 2, 1);

	signalsmith::envelopes::PeakHold<float> peakHoldA(10), peakHoldB(50);

	CsvWriter csv("peak-hold");
	csv.line("i", "signal", "peak (10)", "peak (50)");
	for (int i = 0; i < length; ++i) {
		double v = lfo.next();
		csv.line(i, v, peakHoldA(v), peakHoldB(v));
	}
	return test.pass();
}

TEST("Peak hold (push and pop)", peak_hold_push_pop) {
	int maxLength = 200;
	signalsmith::envelopes::PeakHold<float> peakHold(10);
	peakHold.resize(maxLength);

	std::vector<float> signal(500);
	for (auto &v : signal) v = test.random(-1, 1);
	
	int start = 0, end = 100;
	peakHold.set(0);
	for (int i = 0; i < end; ++i) {
		peakHold.push(signal[i]);
	}
	
	auto check = [&]() {
		float expected = std::numeric_limits<float>::lowest();
		for (int i = start; i < end; ++i) {
			expected = std::max(expected, signal[i]);
		}
		TEST_ASSERT(expected == peakHold.read());
		TEST_ASSERT(peakHold.size() == (end - start));
	};
	
	for (; end < 200; ++end) {
		peakHold.push(signal[end]);
	}
	check();
	for (; start < 150; ++start) {
		peakHold.pop();
	}
	check();
	for (; start < 171; ++start) {
		peakHold.pop();
	}
	check();
	for (; end < 250; ++end) {
		peakHold.push(signal[end]);
	}
	check();
	for (; start < 160; ++start) {
		peakHold.pop();
	}
	check();
}

TEST("Peak hold (push and pop random)", peak_hold_push_pop_random) {
	int maxLength = 200;
	signalsmith::envelopes::PeakHold<float> peakHold(maxLength);

	std::vector<float> signal(5000);
	for (auto &v : signal) {
		double r = test.random(0, 1);
		v = 2*r*r - 1;
	}
	
	peakHold.set(0);
	int start = 0, end = 0;
	auto check = [&]() {
		float expected = std::numeric_limits<float>::lowest();
		for (int i = start; i < end; ++i) {
			expected = std::max(expected, signal[i]);
		}
		
//		// debugging stuff
//		{
//			CsvWriter csv("frames/peak-hold-" + std::to_string(frame++));
//			peakHold._dumpState(csv);
//		}
//		if (expected != peakHold.read()) {
//			CsvWriter csv("peak-hold-error");
//			peakHold._dumpState(csv);
//		}
//		LOG_EXPR(signal[end - 1]);
//		LOG_EXPR(start);
//		LOG_EXPR(end);
//		LOG_EXPR(expected);
//		LOG_EXPR(peakHold.read());

		TEST_ASSERT(expected == peakHold.read());
		TEST_ASSERT(peakHold.size() == (end - start));
	};

	while (1) {
		int newEnd = test.randomInt(end, start + maxLength);
		if (newEnd >= int(signal.size())) break;
		while (end < newEnd) {
			peakHold.push(signal[end]);
			++end;
			check();
		}
		
		int newStart = test.randomInt(start, end - 1);
		while (start < newStart) {
			peakHold.pop();
			++start;
			check();
		}
	}
	
	// Back expands when you resize, including older samples
	for (int repeat = 0; repeat < 50; ++repeat) {
		int newLength = test.randomInt(0, maxLength);
		start = end - newLength;
		if (repeat%2) {
			peakHold.set(newLength);
		} else {
			// Defaults to true, so this is identical
			peakHold.set(newLength, false);
		}
		check();
	}
	
	peakHold.set(50, false);
	float max50 = peakHold.read();
	// Preserve current peak values
	peakHold.set(80, true);
	TEST_ASSERT(peakHold.size() == 80);
	TEST_ASSERT(peakHold.read() == max50);

	peakHold.set(40);
	start = end - 40;
	check();
}

TEST("Peak hold (boundary bug)", peak_hold_boundary_bug) {
	signalsmith::envelopes::PeakHold<float> peakHold(200);
	peakHold.set(0);

	peakHold.push(2);
	for (int i = 1; i < 10; ++i) {
		peakHold.push(1);
	}
	TEST_ASSERT(peakHold.read() == 2);
	peakHold.pop();
	TEST_ASSERT(peakHold.read() == 1);
}

// TODO: test that expanding size re-includes previous values

//TEST("Peak hold (overflow)", peak_hold_overflow) {
//	int maxLength = 200;
//	signalsmith::envelopes::PeakHold<float> peakHold(maxLength);
//
//	std::vector<float> signal(5000);
//	for (auto &v : signal) {
//		v = test.random(-1, 1);
//	}
//
//	// Enough random data to overflow an `int` index
//	for (int r = 0; r < 4; ++r) {
//		LOG_EXPR(r);
//		int intMax = std::numeric_limits<int>::max();
//		for (int i = 0; i < intMax - 10; ++i) {
//			peakHold(test.random(0, 1));
//			if (i%10000000 == 0) {
//				LOG_EXPR(i);
//				LOG_EXPR(i/float(intMax));
//			}
//		}
//	}
//
//	peakHold.set(0);
//	int start = 0, end = 0;
//	auto check = [&]() {
//		float expected = std::numeric_limits<float>::lowest();
//		for (int i = start; i < end; ++i) {
//			expected = std::max(expected, signal[i]);
//		}
//		TEST_ASSERT(expected == peakHold.read());
//		TEST_ASSERT(peakHold.size() == (end - start));
//	};
//
//	while (1) {
//		int newEnd = test.randomInt(end, start + maxLength);
//		if (newEnd >= int(signal.size())) break;
//		while (end < newEnd) {
//			peakHold.push(signal[end]);
//			++end;
//			check();
//		}
//
//		int newStart = test.randomInt(start, end - 1);
//		while (start < newStart) {
//			peakHold.pop();
//			++start;
//			check();
//		}
//	}
//}
