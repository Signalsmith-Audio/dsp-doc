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
	float startingPeak = 0;
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
	signalsmith::envelopes::PeakHold<float> peakHold(maxLength);

	std::vector<float> signal(500);
	for (auto &v : signal) v = test.random(-1, 1);
	
	int start = 0, end = 100;
	peakHold.set(0);
	for (int i = 0; i < end; ++i) {
		peakHold.push(signal[i]);
	}
	
	auto check = [&]() {
		float expected = signal[start];
		for (int i = start; i < end; ++i) {
			expected = std::max(expected, signal[i]);
		}
		LOG_EXPR(expected)
		LOG_EXPR(peakHold.read())
		TEST_ASSERT(expected == peakHold.read());
		LOG_EXPR(peakHold.size())
		LOG_EXPR(start)
		LOG_EXPR(end)
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
	for (; end < 250; ++end) {
		peakHold.push(signal[end]);
	}
	check();
	for (; start < 160; ++start) {
		peakHold.pop();
	}
	check();
}
