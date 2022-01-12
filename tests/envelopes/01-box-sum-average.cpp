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
	signalsmith::envelopes::BoxAverage<double> boxAverage(maxBoxLength);

	for (int i = 0; i < length; ++i) {
		int boxLength = test.randomInt(0, maxBoxLength);
		double result = boxSum.readWrite(signal[i], boxLength);
		boxAverage.set(boxLength);
		double resultAverage = boxAverage.readWrite(signal[i]);
		
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
	boxAverage.reset();
	
	for (int i = 0; i < length; ++i) {
		int boxLength = test.randomInt(0, maxBoxLength);
		boxSum.write(signal[i]);
		double result = boxSum.read(boxLength);
		boxAverage.write(signal[i]);
		boxAverage.set(boxLength);
		double resultAverage = boxAverage.read();

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
}

TEST("Box sum (drift)", box_sum_drift) {
	int maxBoxLength = 100;
	signalsmith::envelopes::BoxSum<float> boxSum(maxBoxLength);
	
	int length = 1000;
	std::vector<float> signal(length);
	
	for (int repeat = 0; repeat < 10; ++repeat) {
		for (int i = 0; i < 10000; ++i) {
			float v = test.random(1e6, 2e6);
			boxSum.write(v);
		}

		for (auto &v : signal) {
			v = test.random(1, -1);
		}
		
		for (int i = 0; i < maxBoxLength; ++i) {
			boxSum.write(signal[i]);
		}

		int boxLength = test.randomInt(25, 100);
		bool foundExactMatch = false;
		int stepsSinceExactMatch = 0;

		for (int i = maxBoxLength; i < length; ++i) {
			boxSum.write(signal[i]);

			float exactSum = 0;
			for (int j = i - boxLength + 1; j <= i; ++j) {
				exactSum += signal[j];
			}
			float result = boxSum.read(boxLength);

			++stepsSinceExactMatch;
			if (result == exactSum) {
				foundExactMatch = true;
				stepsSinceExactMatch = 0;
			}
			// We must get an exact match at least every [buffer size], which should be no longer than 2x the maximum length we requested
			TEST_ASSERT(stepsSinceExactMatch <= maxBoxLength*2);
		}
	}
}
