// from the shared library
#include <test/tests.h>

#include "filters.h"

#include <cmath>

TEST("Filter dB/Q methods") {
	using Filter = signalsmith::filters::BiquadStatic<double>;
	using Design = signalsmith::filters::BiquadDesign;
	
	auto testDesign = [&](Design design, std::string name) {
		Test outerTest = test.prefix(name);

		#define FILTER_METHOD_Q(METHOD) \
		{ \
			Test test = outerTest.prefix(#METHOD); \
			double freq = test.random(0, 0.5); \
			double octaves = test.random(0.1, 4); \
			double q = 0.5/std::sinh(octaves*std::log(2)/2); \
			Filter filter, filterQ; \
			filter.METHOD(freq, octaves, design); \
			filterQ.METHOD##Q(freq, q, design); \
			for (int r = 0; r < 10; ++r) { \
				double f = test.random(0, 0.5); \
				double fDb = filter.responseDb(f); \
				double fqDb = filterQ.responseDb(f); \
				if (fDb > -60 && fqDb > -60) { \
					TEST_APPROX(fDb, fqDb, 0.01); \
				} \
			} \
		}
		FILTER_METHOD_Q(lowpass);
		FILTER_METHOD_Q(highpass);
		FILTER_METHOD_Q(bandpass);
		FILTER_METHOD_Q(notch);
		FILTER_METHOD_Q(allpass);
		#undef FILTER_METHOD_Q

		#define FILTER_METHOD_DB_Q(METHOD) \
		{ \
			Test test = outerTest.prefix(#METHOD); \
			double freq = test.random(0, 0.5); \
			double octaves = test.random(0.1, 4); \
			double q = 0.5/std::sinh(octaves*std::log(2)/2); \
			double db = test.random(-30, 30); \
			double gain = std::pow(10, db*0.05); \
			Filter filter, filterQ, filterDb, filterDbQ; \
			filter.METHOD(freq, gain, octaves, design); \
			filterQ.METHOD##Q(freq, gain, q, design); \
			filterDb.METHOD##Db(freq, db, octaves, design); \
			filterDbQ.METHOD##DbQ(freq, db, q, design); \
			for (int r = 0; r < 10; ++r) { \
				double f = test.random(0, 0.5); \
				double fDb = filter.responseDb(f); \
				double fqDb = filterQ.responseDb(f); \
				double fDbDb = filterDb.responseDb(f); \
				double fDbQDb = filterDbQ.responseDb(f); \
				if (fDb > -60 && fqDb > -60) { \
					TEST_APPROX(fDb, fqDb, 0.01); \
					TEST_APPROX(fDb, fDbDb, 0.01); \
					TEST_APPROX(fDb, fDbQDb, 0.01); \
				} \
			} \
		}
		FILTER_METHOD_DB_Q(peak);
		FILTER_METHOD_DB_Q(highShelf);
		FILTER_METHOD_DB_Q(lowShelf);
		#undef FILTER_METHOD_DB_Q
	};
	
	testDesign(Design::bilinear, "bilinear");
	// We don't test "cookbook" because that's the one with the weird Q/bandwidth relationship
	testDesign(Design::oneSided, "oneSided");
	testDesign(Design::vicanek, "vicanek");
}
