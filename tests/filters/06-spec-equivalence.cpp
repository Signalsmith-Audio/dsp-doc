// from the shared library
#include <test/tests.h>

#include "filters.h"

#include <cmath>

TEST("Filter dB/Q methods", filters_db_q) {
	using Filter = signalsmith::filters::BiquadStatic<double>;
	using Design = signalsmith::filters::BiquadDesign;
	Test &outerTest = test;

	#define FILTER_METHOD_Q(METHOD) \
	{ \
		Test test = outerTest.prefix(#METHOD); \
		double freq = test.random(0, 0.5); \
		double octaves = test.random(0.1, 4); \
		double q = 0.5/std::sinh(octaves*std::log(2)/2); \
		Filter filter, filterQ; \
		filter.METHOD(freq, octaves); \
		filterQ.METHOD##Q(freq, q); \
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

	#define FILTER_METHOD_DB_Q(METHOD) \
	{ \
		Test test = outerTest.prefix(#METHOD); \
		double freq = test.random(0, 0.5); \
		double octaves = test.random(0.1, 4); \
		double q = 0.5/std::sinh(octaves*std::log(2)/2); \
		double db = test.random(-30, 30); \
		double gain = std::pow(10, db*0.05); \
		Filter filter, filterQ, filterDb, filterDbQ; \
		filter.METHOD(freq, gain, octaves); \
		filterQ.METHOD##Q(freq, gain, q); \
		filterDb.METHOD##Db(freq, db, octaves); \
		filterDbQ.METHOD##DbQ(freq, db, q); \
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
}

