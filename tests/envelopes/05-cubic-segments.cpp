#include <test/tests.h>
#include "../common.h"

#include "envelopes.h"

#include <array>

TEST("Cubic segments (gradient)", segment_gradient) {
	using Segment = signalsmith::envelopes::CubicSegments<double>::Segment;
	double x0 = test.random(-1, 1);

	Segment s(
		x0,
		test.random(-1, 1),
		test.random(-1, 1),
		test.random(-1, 1),
		test.random(-1, 1)
	);
	Segment grad = s.dx();
	
	for (int r = 0; r < 100; ++r) {
		double x = test.random(x0, x0 + 2);
		double v = s(x);
		double dx = 1e-10;
		double v2 = s(x + dx);
		double approxGrad = (v2 - v)/dx;
		double g = grad(x);

		// Just ballpark correct
		double diff = std::abs(approxGrad - g);
		TEST_ASSERT(diff < 1e-4);
	}
}

TEST("Cubic segments (hermite)", segment_hermite) {
	using Segment = signalsmith::envelopes::CubicSegments<double>::Segment;

	for (int r = 0; r < 100; ++r) {
		double x0 = test.random(-1, 1);
		double x1 = x0 + test.random(0.01, 2);
		double y0 = test.random(-10, 10);
		double y1 = test.random(-10, 10);
		double g0 = test.random(-5, 5);
		double g1 = test.random(-5, 5);

		Segment s = Segment::hermite(x0, x1, y0, y1, g0, g1);
		Segment grad = s.dx();
		TEST_ASSERT(std::abs(s(x0) - y0) < 1e-6);
		TEST_ASSERT(std::abs(s(x1) - y1) < 1e-6);
		TEST_ASSERT(std::abs(grad(x0) - g0) < 1e-6);
		TEST_ASSERT(std::abs(grad(x1) - g1) < 1e-6);
	}
}

TEST("Cubic segments (known)", segment_known) {
	using Curve = signalsmith::envelopes::CubicSegments<double>;
	using Segment = Curve::Segment;

	{
		Segment s = Curve::fromPoints(0, 1, 2, 3, 0, 1, 2, 3);
		TEST_ASSERT(std::abs(s(0) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(0) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s(1.5) - 1.5) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1.5) - 1) < 1e-6);
	}
	{
		Segment s = Curve::fromPoints(0, 1, 2, 3, 0, 1, 0, 1);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 0) < 1e-6);
	}

	{ // monotonic
		Segment s = Curve::fromPoints(0, 1, 2, 3, -1, 1, 0, 2, true);
		TEST_ASSERT(std::abs(s(1) - 1) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(1) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s(2) - 0) < 1e-6);
		TEST_ASSERT(std::abs(s.dx()(2) - 0) < 1e-6);
	}
}

TEST("Cubic segments (random)", segment_random) {
	using Curve = signalsmith::envelopes::CubicSegments<double>;
	using Segment = Curve::Segment;
	
	for (int r = 0; r < 100; ++r) {
		bool monotonic = r%2;
		
		std::array<double, 5> x, y;
		x[0] = test.random(-1, 1);
		for (int i = 1; i < 5; ++i) x[i] = x[i - 1] + test.random(1e-10, 2);
		for (auto &v : y) v = test.random(-10, 10);
		
		Segment sA = Curve::fromPoints(x[0], x[1], x[2], x[3], y[0], y[1], y[2], y[3], monotonic);
		Segment sB = Curve::fromPoints(x[1], x[2], x[3], x[4], y[1], y[2], y[3], y[4], monotonic);
		
		// Test smoothness - their gradients agree at x2
		TEST_ASSERT(std::abs(sA.dx()(x[2]) - sB.dx()(x[2])) < 1e-6);
		
		if (monotonic) {
			if (y[1] >= y[2]) { // decreasing
				double min = sA(x[1]);
				for (double t = x[1]; t <= x[2]; t += 1e-3) {
					double y = sA(t);
					min = y;
				}
			}
			if (y[1] <= y[2]) { // increasing
				double max = sA(x[1]);
				for (double t = x[1]; t <= x[2]; t += 1e-3) {
					double y = sA(t);
					TEST_ASSERT(y >= max);
					max = y;
				}
			}
		}
	}
}
