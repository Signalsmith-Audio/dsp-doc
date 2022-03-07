#include <test/tests.h>
#include "../common.h"

#include "curves.h"

#include <cmath>

TEST("Cubic segments (gradient)", segment_gradient) {
	using Linear = signalsmith::curves::Linear<float>;
	float accuracy = 1e-3;
	
	for (int r = 0; r < 100; ++r) {
		float x0 = test.random(-10, 10), x1 = test.random(-10, 10);
		float y0 = test.random(-10, 10), y1 = test.random(-10, 10);

		Linear linear{x0, x1, y0, y1};

		TEST_ASSERT(std::abs(linear(x0) - y0) < accuracy);
		TEST_ASSERT(std::abs(linear(x1) - y1) < accuracy);

		Linear inverse = linear.inverse();
		TEST_ASSERT(std::abs(inverse(y0) - x0) < accuracy*2);
		TEST_ASSERT(std::abs(inverse(y1) - x1) < accuracy*2);
	}
}
