#include <test/tests.h>
#include "../common.h"

#include <array>
#include <cmath>

#include "envelopes.h"

TEST("Box sum", box_sum) {
	int length = 1000;
	std::vector<double> signal(length);
	for (auto &v : signal) {
		v = test.random(-1, 1);
	}

	int maxBoxLength = 100;
	signalsmith::envelopes::BoxSum<double> boxSum(maxBoxLength);
	signalsmith::envelopes::BoxFilter<double> boxFilter(maxBoxLength);

	for (int i = 0; i < length; ++i) {
		int boxLength = test.randomInt(0, maxBoxLength);
		double result = boxSum.readWrite(signal[i], boxLength);
		boxFilter.set(boxLength);
		double resultAverage = boxFilter(signal[i]);
		
		int start = std::max(0, i - boxLength + 1);
		double sum = (boxLength ? signal[i] : 0);
		for (int j = start; j < i; ++j) {
			sum += signal[j];
		}
		
		double diff = result - sum;
		TEST_ASSERT(std::abs(diff) < 1e-12);

		if (boxLength > 0) {
			double diffAvg = resultAverage - sum/boxLength;
			TEST_ASSERT(std::abs(diffAvg) < 1e-12);
		}
	}
	
	boxSum.reset();
	boxFilter.reset();
	
	for (int i = 0; i < length; ++i) {
		int boxLength = test.randomInt(0, maxBoxLength);
		boxSum.write(signal[i]);
		double result = boxSum.read(boxLength);

		int start = std::max(0, i - boxLength + 1);
		double sum = (boxLength ? signal[i] : 0);
		for (int j = start; j < i; ++j) {
			sum += signal[j];
		}
		
		double diff = result - sum;
		TEST_ASSERT(std::abs(diff) < 1e-12);
	}
}

TEST("Box sum (drift)", box_sum_drift) {
	int maxBoxLength = 100;
	signalsmith::envelopes::BoxSum<float> boxSum(maxBoxLength);
	
	for (int repeat = 0; repeat < 10; ++repeat) {
		for (int i = 0; i < 10000; ++i) {
			float v = test.random(1e6, 2e6);
			boxSum.write(v);
		}

		for (int i = 0; i < maxBoxLength*2; ++i) {
			float v = (i&1) ? 1 : -1;
			boxSum.write(v);
		}

		for (int r = 0; r < 10; ++r) {
			int boxLength = test.randomInt(25, 100);
			float expected = (boxLength%2) ? 1 : 0;
			float actual = boxSum.read(boxLength);
			
			TEST_ASSERT(expected == actual); // Should be exact match
		}
	}
}
