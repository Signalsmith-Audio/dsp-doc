#include <test/tests.h>
#include "../common.h"

#include "curves.h"

#include <cmath>

TEST("Linear segments", linear_segment) {
	using Linear = signalsmith::curves::Linear<float>;
	float accuracy = 1e-3;
	
	for (int repeat = 0; repeat < 100; ++repeat) {
		float x0 = test.random(-10, 10), x1 = test.random(-10, 10);
		float y0 = test.random(-10, 10), y1 = test.random(-10, 10);

		Linear linear{x0, x1, y0, y1};

		TEST_ASSERT(std::abs(linear(x0) - y0) < accuracy);
		TEST_ASSERT(std::abs(linear(x1) - y1) < accuracy);

		double r = test.random(0, 1);
		double rx = x0 + (x1 - x0)*r;
		double ry = y0 + (y1 - y0)*r;
		TEST_ASSERT(std::abs(linear(rx) - ry) < accuracy);

		Linear inverse = linear.inverse();
		TEST_ASSERT(std::abs(inverse(y0) - x0) < accuracy*2);
		TEST_ASSERT(std::abs(inverse(y1) - x1) < accuracy*2);

		TEST_ASSERT(std::abs(inverse(ry) - rx) < accuracy);
	}
}
