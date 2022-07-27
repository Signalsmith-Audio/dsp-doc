// from the shared library
#include <test/tests.h>

#include "filters.h"

#include "../common.h"

TEST("Shelf (bandwidth)", filters_shelf_bandwidth) {
	using Filter = signalsmith::filters::BiquadStatic<double>;
	for (int r = 0; r < 100; ++r) {
		double freq = test.random(0.001, 0.49);
		double db = test.random(-10, 10);
		double octaves = test.random(0.5, 3);
		
		Filter highShelf, lowShelf;
		highShelf.highShelfDb(freq, db, octaves);
		lowShelf.lowShelfDb(freq, db, octaves);
		
		TEST_APPROX(highShelf.responseDb(0), 0, 0.01);
		TEST_APPROX(lowShelf.responseDb(0), db, 0.01);
		TEST_APPROX(highShelf.responseDb(freq), db/2, 0.01);
		TEST_APPROX(lowShelf.responseDb(freq), db/2, 0.01);
		TEST_APPROX(highShelf.responseDb(0.5), db, 0.01);
		TEST_APPROX(lowShelf.responseDb(0.5), 0, 0.01);
	}
}

TEST("Shelf (max slope)", filters_shelf_slope) {
	using Filter = signalsmith::filters::BiquadStatic<double>;
	using Design = signalsmith::filters::BiquadDesign;
	for (int r = 0; r < 100; ++r) {
		double freq = test.random(0.01, 0.49);
		double db = test.random(-10, 10);
		while (std::abs(db) < 0.1) db = test.random(-10, 10);
		
		auto isMonotonic = [&](const Filter &filter, int direction) -> bool {
			double accuracy = 1e-6;
			double min = filter.responseDb(0), max = min;
			for (int i = 0; i < 1000; ++i) {
				double f = (i + test.random(0, 1))/2000;
				double rDb = filter.responseDb(f);
				
				if (direction > 0) {
					if (rDb < min - accuracy) {
						return false;
					}
				} else {
					if (rDb > max + accuracy) {
						return false;
					}
				}
				min = std::min(min, rDb);
				max = std::max(min, rDb);
			}
			return true;
		};
		
		Filter highShelf, lowShelf;

		// Monotonic with default values
		highShelf.highShelfDb(freq, db, highShelf.defaultBandwidth, Design::bilinear);
		lowShelf.lowShelfDbQ(freq, db, lowShelf.defaultQ, Design::bilinear);
		if (db > 0) {
			TEST_ASSERT(isMonotonic(highShelf, 1));
			TEST_ASSERT(isMonotonic(lowShelf, -1));
		} else {
			TEST_ASSERT(isMonotonic(highShelf, -1));
			TEST_ASSERT(isMonotonic(lowShelf, 1));
		}

		// Loosening bandwidth is still monotonic
		highShelf.highShelfDb(freq, db, highShelf.defaultBandwidth*1.05, Design::bilinear);
		lowShelf.lowShelfDbQ(freq, db, lowShelf.defaultQ*0.95, Design::bilinear);
		if (db > 0) {
			TEST_ASSERT(isMonotonic(highShelf, 1));
			TEST_ASSERT(isMonotonic(lowShelf, -1));
		} else {
			TEST_ASSERT(isMonotonic(highShelf, -1));
			TEST_ASSERT(isMonotonic(lowShelf, 1));
		}
		
		// Narrower bandwidth / higher Q makes it overshoot
		highShelf.highShelfDb(freq, db, highShelf.defaultBandwidth*0.95, Design::bilinear);
		lowShelf.lowShelfDbQ(freq, db, lowShelf.defaultQ*1.05, Design::bilinear);
		if (db > 0) {
			TEST_ASSERT(!isMonotonic(highShelf, 1));
			TEST_ASSERT(!isMonotonic(lowShelf, -1));
		} else {
			TEST_ASSERT(!isMonotonic(highShelf, -1));
			Plot2D plot(600, 400);
			auto &line = plot.line();
			for (double f = 0; f < 0.5; f += 0.001) {
				line.add(f, lowShelf.responseDb(f));
			}
			plot.write("tmp.svg");
			TEST_ASSERT(!isMonotonic(lowShelf, 1));
		}
		
	}
}

