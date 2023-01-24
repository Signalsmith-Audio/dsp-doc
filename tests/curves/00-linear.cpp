#include <test/tests.h>
#include "../common.h"

#include "curves.h"

#include <cmath>

TEST("Linear segments") {
	using Linear = signalsmith::curves::Linear<float>;
	float accuracy = 1e-4;
	
	for (int repeat = 0; repeat < 100; ++repeat) {
		float x0 = test.random(-10, 10), x1 = test.random(-10, 10);
		float y0 = test.random(-10, 10), y1 = test.random(-10, 10);
		float accuracyX = accuracy/std::abs(x1 - x0); // gets less accurate when the input points are closer together
		float accuracyY = accuracy/std::abs(y1 - y0);

		const Linear linear{x0, x1, y0, y1};

		TEST_ASSERT(std::abs(linear(x0) - y0) < accuracyX);
		TEST_ASSERT(std::abs(linear(x1) - y1) < accuracyX);

		double r = test.random(0, 1);
		double rx = x0 + (x1 - x0)*r;
		double ry = y0 + (y1 - y0)*r;
		TEST_ASSERT(std::abs(linear(rx) - ry) < accuracyX);

		Linear inverse = linear.inverse();
		TEST_ASSERT(std::abs(inverse(y0) - x0) < accuracyY);
		TEST_ASSERT(std::abs(inverse(y1) - x1) < accuracyY);

		TEST_ASSERT(std::abs(inverse(ry) - rx) < accuracyY);
		
		double gradient = (y1 - y0)/(x1 - x0);
		TEST_APPROX(linear.dx(), gradient, accuracyX);
	}
}
