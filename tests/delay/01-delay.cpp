// from the shared library
#include <test/tests.h>

#include "delay.h"
#include "test-delay-stats.h"

#include <iostream>

TEST("Reader") {
	int delaySize = 100;
	signalsmith::delay::Buffer<double> buffer(delaySize);
	signalsmith::delay::Reader<double> reader;
	
	// Default reader/interpolator has no latency
	TEST_ASSERT(reader.latency == 0);
	
	for (int i = 0; i < delaySize; ++i) {
		buffer[-i] = i;
	}
	// Integer delays give us the correct result
	for (int i = 0; i < delaySize; ++i) {
		double delaySamples = i;
		TEST_ASSERT(reader.read(buffer, delaySamples) == i);
	}
}

TEST("Delay") {
	int delayLength = 127;
	
	signalsmith::delay::Delay<double> multiDelay(delayLength);
	
	// Put a known sequence in
	for (int i = 0; i < delayLength; ++i) {
		multiDelay.write(i);
	}

	// Read out delayed samples
	for (int i = 0; i < delayLength; ++i) {
		double value = multiDelay.write(0).read(delayLength);
		TEST_ASSERT(value == i);
	}
}


TEST("Delay: nearest interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorNearest> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-nearest");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-2, -3, -6.5, -12.5, -18};
	double ampLow[] = {-0.1, -0.1, -0.1, -0.1, -0.1};
	double ampHigh[] = {0.1, 0.1, 0.1, 0.1, 0.1};
	double delayError[] = {1, 1, 1, 1, 1};

	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: linear interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorLinear> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-linear");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-9, -11, -19.5, -32, -44};
	double ampLow[] = {-16, -10.5, -3.5, -0.7, -0.2};
	double ampHigh[] = {0, 0, 0, 0, 0};
	double delayError[] = {1.5, 0.7, 0.15, 0.04, 0.01};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: cubic interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorCubic> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-cubic");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-8, -11, -24, -43.5, -62.5};
	double ampLow[] = {-13, -7, -1.1, -0.08, -0.01};
	double ampHigh[] = {0.01, 0.01, 0.01, 0.01, 0.01};
	double delayError[] = {1.5, 0.7, 0.15, 0.04, 0.01};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Lagrange-3 interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorLagrange3> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-lagrange3");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-9.5, -12.5, -27.5, -51, -74.5};
	double ampLow[] = {-13, -7, -1.1, -0.08, -0.01};
	double ampHigh[] = {0.01, 0.01, 0.01, 0.01, 0.01};
	double delayError[] = {1.5, 0.7, 0.09, 0.01, 0.01};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Lagrange-7 interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorLagrange7> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-lagrange7");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-10.5, -15.5, -41.5, -86.5, -134};
	double ampLow[] = {-9.5, -4.5, -0.2, -0.01, -0.01};
	double ampHigh[] = {0.01, 0.01, 0.01, 0.01, 0.01};
	double delayError[] = {1.5, 0.6, 0.03, 0.01, 0.01};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Lagrange-19 interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorLagrange19> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-lagrange19");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-13, -22.5, -81, -150, -150};
	double ampLow[] = {-6, -2, -0.01, -0.01, -0.01};
	double ampHigh[] = {0.01, 0.01, 0.01, 0.01, 0.01};
	double delayError[] = {1.5, 0.35, 0.01, 0.01, 0.01};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Sinc4 interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorKaiserSinc4> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-sinc4");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-9, -13, -33, -36.5, -37};
	double ampLow[] = {-10.5, -5, -0.4, -0.4, -0.4};
	double ampHigh[] = {0.45, 0.45, 0.45, 0.2, 0.0001};
	double delayError[] = {1.5, 0.7, 0.09, 0.03, 0.03};
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Sinc8 interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorKaiserSinc8> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-sinc8");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-11.5, -21, -46.5, -50, -51.5};
	double ampLow[] = {-6.5, -2, -0.1, -0.08, -0.08};
	double ampHigh[] = {0.15, 0.15, 0.08, 0.08, 0.02};
	double delayError[] = {1.5, 0.5, 0.03, 0.02, 0.02};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Sinc20 interpolation") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorKaiserSinc20> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-sinc20");

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-20.5, -61, -70.5, -76, -77.5};
	double ampLow[] = {-2, -0.02, -0.01, -0.01, -0.01};
	double ampHigh[] = {0.02, 0.02, 0.01, 0.01, 0.01};
	double delayError[] = {0.9, 0.03, 0.01, 0.01, 0.01};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Sinc4 interpolation (min-phase)") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorKaiserSinc4Min> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-sinc4min", false);

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-9, -13.5, -33, -33.5, -33.5};
	double ampLow[] = {-10.5, -5.5, -0.8, -0.8, -0.8};
	double ampHigh[] = {0.4, 0.4, 0.4, 0.2, 0.03};
	double delayError[] = {2, 1.2, 0.3, 0.2, 0.2};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Sinc8 interpolation (min-phase)") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorKaiserSinc8Min> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-sinc8min", false);

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-12, -21, -45, -48, -49};
	double ampLow[] = {-6.5, -2, -0.12, -0.09, -0.09};
	double ampHigh[] = {0.1, 0.1, 0.08, 0.08, 0.04};
	double delayError[] = {3.5, 2, 0.6, 0.4, 0.35};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}

TEST("Delay: Sinc20 interpolation (min-phase)") {
	signalsmith::delay::Delay<double, signalsmith::delay::InterpolatorKaiserSinc20Min> delay;
	auto result = collectFractionalDelayStats<double>(test, delay, "delay-random-access-sinc20min", false);

	// A table of acceptable limits
	double bandwidth[] = {90, 80, 50, 25, 12.5};
	double aliasing[] = {-20.5, -61.5, -71, -76, -77.5};
	double ampLow[] = {-2, -0.06, -0.01, -0.01, -0.01};
	double ampHigh[] = {0.01, 0.01, 0.01, 0.01, 0.01};
	double delayError[] = {5.5, 3, 1.5, 1, 1};
	
	result.test(test, bandwidth, aliasing, ampLow, ampHigh, delayError);
}
