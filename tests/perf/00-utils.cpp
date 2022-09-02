// from the shared library
#include <test/tests.h>
#include <stopwatch.h>

#include "../common.h"
#include "perf.h"

#include <complex>

template<typename Sample>
void testComplex(Test &test) {
	using Complex = std::complex<Sample>;
	auto rand = [&](){return Sample(test.random(-100, 100));};

	int repeats = 1000;
	for (int r = 0; r < repeats; ++r) {
		Complex a = {rand(), rand()};
		Complex b = {rand(), rand()};
		
		if (a*b != signalsmith::perf::mul(a, b)) return test.fail("multiplication");
		if (a*std::conj(b) != signalsmith::perf::mul<true>(a, b)) return test.fail("multiplication");
	}
}

TEST("Complex multiplcation") {
	testComplex<float>(test);
	testComplex<double>(test);
}
