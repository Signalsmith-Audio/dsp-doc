#include <test/tests.h>
#include "../common.h"

#include "curves.h"

#include <cmath>

TEST("Reciprocal correctly maps") {
	double accuracy = 1e-6;
	
	for (int repeat = 0; repeat < 100; ++repeat) {
		double x0 = test.random(-100, 100), x1 = test.random(-100, 100);
		double y0 = test.random(-100, 100), y1 = test.random(-100, 100);
		double xm = x0 + (x1 - x0)*test.random(0, 1);
		double ym = y0 + (y1 - y0)*test.random(0, 1);

		signalsmith::curves::Reciprocal<double> reciprocal(x0, xm, x1, y0, ym, y1);
		TEST_ASSERT(std::abs(reciprocal(x0) - y0) < accuracy);
		TEST_ASSERT(std::abs(reciprocal(x1) - y1) < accuracy);
		TEST_ASSERT(std::abs(reciprocal(xm) - ym) < accuracy);
		
		// Test it's monotonic
		bool increasing = (y1 > y0);
		double prevY = y0;
		for (double r = 0.01; r < 1; r += 0.01) {
			double x = x0 + (x1 - x0)*r;
			double y = reciprocal(x);
			TEST_ASSERT((y > prevY) == increasing);
			prevY = y;
		}
	}
}

TEST("Reciprocal inverse") {
	double accuracy = 1e-6;
	
	for (int repeat = 0; repeat < 10; ++repeat) {
		double x0 = test.random(-100, 100), x1 = test.random(-100, 100);
		double y0 = test.random(-100, 100), y1 = test.random(-100, 100);
		double xm = x0 + (x1 - x0)*test.random(0, 1);
		double ym = y0 + (y1 - y0)*test.random(0, 1);

		signalsmith::curves::Reciprocal<double> reciprocal(x0, xm, x1, y0, ym, y1);
		signalsmith::curves::Reciprocal<double> inverse = reciprocal.inverse();
		
		for (int repeat2 = 0; repeat2 < 10; ++repeat2) {
			double x = x0 + (x1 - x0)*test.random(0, 1);
			double y = reciprocal(x);
			TEST_ASSERT(std::abs(inverse(y) - x) < accuracy)
			TEST_ASSERT(inverse(y) == reciprocal.inverse(y))
		}
	}
}

TEST("Reciprocal compose") {
	double accuracy = 1e-6;
	
	for (int repeat = 0; repeat < 10; ++repeat) {
		
		double x0 = test.random(-100, 100), x1 = test.random(-100, 100);
		double y0 = test.random(-100, 100), y1 = test.random(-100, 100);
		double z0 = test.random(-100, 100), z1 = test.random(-100, 100);
		double xm = x0 + (x1 - x0)*test.random(0, 1);
		double ym = y0 + (y1 - y0)*test.random(0, 1);
		double zm = z0 + (z1 - z0)*test.random(0, 1);

		signalsmith::curves::Reciprocal<double> reciprocalA(x0, xm, x1, y0, ym, y1);
		signalsmith::curves::Reciprocal<double> reciprocalB(y0, ym, y1, z0, zm, z1);
		signalsmith::curves::Reciprocal<double> composite = reciprocalA.then(reciprocalB);
		
		for (int repeat2 = 0; repeat2 < 10; ++repeat2) {
			double x = x0 + (x1 - x0)*test.random(0, 1);
			double y = reciprocalA(x);
			double z = reciprocalB(y);
			double zC = composite(x);
			TEST_ASSERT(std::abs(zC - z) < accuracy)
		}
	}
}

TEST("Reciprocal (example)") {
	Plot2D plot(200, 200);
	plot.x.linear(-2, 1).major(-2, "").label("input");
	plot.y.linear(0, 10).major(0, "").label("output");
	auto addLine = [&](double xm, double ym) {
		signalsmith::curves::Reciprocal<double> reciprocal(-2, xm, 1, 0, ym, 10);
		auto &line = plot.line();
		
		for (double x = -2; x < 1; x += 0.001) {
			line.add(x, reciprocal(x));
		}
		line.styleIndex.marker = 0;
		line.marker(xm, ym);
	};
	addLine(0.2, 0.5);
	addLine(-1, 2);
	addLine(-0.5, 8);
	addLine(-1.5, 9.5);
	
	plot.write("curves-reciprocal-example.svg");
	return test.pass();
}
