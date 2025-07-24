// from the shared library
#include <test/tests.h>
#include <stopwatch.h>

#include "../common.h"
#include "perf.h"

#include <complex>

template<typename Sample>
void testComplex(Test &test, Sample errorLimit=1e-5) {
	using Complex = std::complex<Sample>;
	auto rand = [&](){return Sample(test.random(-100, 100));};

	int repeats = 1000;
	for (int r = 0; r < repeats; ++r) {
		Complex a = {rand(), rand()};
		Complex b = {rand(), rand()};
		
		{
			auto expected = a*b;
			auto actual = signalsmith::perf::mul(a, b);
			auto limit = errorLimit*(std::abs(expected) + Sample(1e-2));
			if (std::abs(expected - actual) > limit) return test.fail("multiplication");
		}
		{
			auto expected = a*std::conj(b);
			auto actual = signalsmith::perf::mul<true>(a, b);
			auto limit = errorLimit*(std::abs(expected) + Sample(1e-2));
			if (std::abs(expected - actual) > limit) return test.fail("conjugate multiplication");
		}
	}
}

TEST("Complex multiplcation") {
	testComplex<float>(test, 1e-6);
	testComplex<double>(test, 1e-12);
}
