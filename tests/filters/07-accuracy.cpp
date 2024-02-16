// from the shared library
#include <test/tests.h>

#include "filters.h"

#include <cmath>

TEST("Double/float accuracy") {
	using FilterDouble = signalsmith::filters::BiquadStatic<double>;
	using FilterFloat = signalsmith::filters::BiquadStatic<float>;
	using Design = signalsmith::filters::BiquadDesign;

	int repeats = 100;
	
	double totalError2 = 0;
	int totalErrorCounter = 0;

	auto testDesign = [&](Design design, std::string name, double errorLimit) {
		Test outerTest = test.prefix(name);
		
		auto testAccuracy = [&](FilterDouble &filterDouble, FilterFloat &filterFloat, double &diffTotal2, double &outTotal2) {
			filterDouble.reset();
			filterFloat.reset();
			double zeroLimit = errorLimit;
			int endI = 100;
			for (int i = 0; i < endI; ++i) {
				double v = (i == 0);
				double outDouble = filterDouble(v), outFloat = filterFloat(v);
				if (std::abs(outDouble) > zeroLimit) endI = std::max(i*2, endI); // continue as long as the tail does
				double diff = outDouble - outFloat;
				diffTotal2 += diff*diff;
				outTotal2 += outDouble*outDouble;
				if (std::to_string(diffTotal2) == "nan") { \
					/* make public in `filters.h` if you get here
					TEST_EXPR(filterDouble.a1); \
					TEST_EXPR(filterFloat.a1); \
					TEST_EXPR(filterDouble.a2); \
					TEST_EXPR(filterFloat.a2); \
					TEST_EXPR(filterDouble.b0); \
					TEST_EXPR(filterFloat.b0); \
					TEST_EXPR(filterDouble.b1); \
					TEST_EXPR(filterFloat.b1); \
					TEST_EXPR(filterDouble.b2); \
					TEST_EXPR(filterFloat.b2); \
					TEST_EXPR(filterDouble.x1); \
					TEST_EXPR(filterFloat.x1); \
					TEST_EXPR(filterDouble.x2); \
					TEST_EXPR(filterFloat.x2); \
					TEST_EXPR(i); \
					TEST_EXPR(outDouble); \
					TEST_EXPR(outFloat); \
					TEST_EXPR(diffTotal2); \
					TEST_EXPR(outTotal2); \
					*/
					abort(); \
				} \
			}
		};

		#define FILTER_METHOD(METHOD) \
		{ \
			Test test = outerTest.prefix(#METHOD); \
			double diffTotal2 = 0; \
			double outTotal2 = 0; \
			for (int repeat = 0; repeat < repeats; ++repeat) { \
				double freq = 0.5*(repeat + 0.5)/repeats; \
				double octaves = test.random(0.1, 4); \
				FilterDouble filterDouble; \
				FilterFloat filterFloat; \
				filterDouble.METHOD(freq, octaves, design); \
				filterFloat.METHOD(freq, octaves, design); \
				testAccuracy(filterDouble, filterFloat, diffTotal2, outTotal2); \
			} \
			double rmsError = std::sqrt(diffTotal2/outTotal2); \
			if (rmsError >= errorLimit) { \
				TEST_EXPR(rmsError); \
				return test.fail("float/double error too high"); \
			} \
			totalError2 += rmsError*rmsError; \
			++totalErrorCounter; \
		}
		FILTER_METHOD(lowpass);
		FILTER_METHOD(highpass);
		FILTER_METHOD(bandpass);
		FILTER_METHOD(notch);
		FILTER_METHOD(allpass);
		#undef FILTER_METHOD

		#define FILTER_METHOD_DB(METHOD) \
		{ \
			Test test = outerTest.prefix(#METHOD); \
			double diffTotal2 = 0; \
			double outTotal2 = 0; \
			for (int repeat = 0; repeat < repeats; ++repeat) { \
				double freq = 0.5*(repeat + 0.5)/repeats; \
				double octaves = test.random(0.5, 4); \
				double db = test.random(-30, 30); \
				FilterDouble filterDouble; \
				FilterFloat filterFloat; \
				filterDouble.METHOD##Db(freq, db, octaves, design); \
				filterFloat.METHOD##Db(freq, db, octaves, design); \
				testAccuracy(filterDouble, filterFloat, diffTotal2, outTotal2); \
			} \
			double rmsError = std::sqrt(diffTotal2/outTotal2); \
			if (rmsError >= errorLimit) { \
				TEST_EXPR(rmsError); \
				return test.fail("float/double error too high"); \
			} \
			totalError2 += rmsError*rmsError; \
			++totalErrorCounter; \
		}
		FILTER_METHOD_DB(peak);
		FILTER_METHOD_DB(highShelf);
		FILTER_METHOD_DB(lowShelf);
		#undef FILTER_METHOD_DB
	};
	
	testDesign(Design::bilinear, "bilinear", 2e-4);
	testDesign(Design::cookbook, "cookbook", 2e-4);
	testDesign(Design::oneSided, "oneSided", 2e-4);
	testDesign(Design::vicanek, "vicanek", 2e-4);
	
	double totalRmsError = std::sqrt(totalError2/totalErrorCounter);
	TEST_EXPR(totalRmsError);
}
