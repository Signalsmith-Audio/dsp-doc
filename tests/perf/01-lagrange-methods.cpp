// from the shared library
#include <test/tests.h>
#include <stopwatch.h>

#include "delay.h"
#include "../common.h"

#include <iostream>
#include <string>

/*////// Graveyard of Lagrange implementations //////*/

template<typename Sample, int n>
struct LagrangeMulAddBasic {
	static constexpr int inputLength = n + 1;
	static constexpr int latency = (n - 1)/2;

	std::array<Sample, (n + 1)> invDivisors;
	
	LagrangeMulAddBasic() {
		for (int j = 0; j <= n; ++j) {
			double divisor = 1;
			for (int k = 0; k < j; ++k) divisor *= (j - k);
			for (int k = j + 1; k <= n; ++k) divisor *= (j - k);
			invDivisors[j] = 1/divisor;
		}
	}
	
	template<class Data>
	Sample fractional(const Data &data, Sample fractional) const {
		std::array<Sample, (n + 1)> sums;
		
		Sample x = fractional + latency;

		Sample forwardFactor = 1;
		sums[0] = data[0];
		for (int i = 1; i <= n; ++i) {
			forwardFactor *= x - (i - 1);
			sums[i] = forwardFactor*data[i];
		}
		
		Sample backwardsFactor = 1;
		Sample result = sums[n]*invDivisors[n];
		for (int i = n - 1; i >= 0; --i) {
			backwardsFactor *= x - (i + 1);
			result += sums[i]*invDivisors[i]*backwardsFactor;
		}
		return result;
	}
};

namespace _franck_impl {
	template<typename Sample, int n, int low, int high>
	struct ProductRange {
		using Array = std::array<Sample, (n + 1)>;
		static constexpr int mid = (low + high)/2;
		using Left = ProductRange<Sample, n, low, mid>;
		using Right = ProductRange<Sample, n, mid + 1, high>;

		Left left;
		Right right;
		
		const Sample total;
		ProductRange(Sample x) : left(x), right(x), total(left.total*right.total) {}
		
		template<class Data>
		Sample calculateResult(Sample extraFactor, const Data &data, const Array &invFactors) {
			return left.calculateResult(extraFactor*right.total, data, invFactors)
				+ right.calculateResult(extraFactor*left.total, data, invFactors);
		}
	};
	template<typename Sample, int n, int index>
	struct ProductRange<Sample, n, index, index> {
		using Array = std::array<Sample, (n + 1)>;

		const Sample total;
		ProductRange(Sample x) : total(x - index) {}
		
		template<class Data>
		Sample calculateResult(Sample extraFactor, const Data &data, const Array &invFactors) {
			return extraFactor*data[index]*invFactors[index];
		}
	};
}

template<typename Sample, int n>
struct LagrangeMulAddFranck {
	static constexpr int inputLength = n + 1;
	static constexpr int latency = (n - 1)/2;

	using Array = std::array<Sample, (n + 1)>;
	Array invDivisors;
	
	LagrangeMulAddFranck() {
		for (int j = 0; j <= n; ++j) {
			double divisor = 1;
			for (int k = 0; k < j; ++k) divisor *= (j - k);
			for (int k = j + 1; k <= n; ++k) divisor *= (j - k);
			invDivisors[j] = 1/divisor;
		}
	}
	
	template<class Data>
	Sample fractional(const Data &data, Sample fractional) const {
		constexpr int mid = n/2;
		using Left = _franck_impl::ProductRange<Sample, n, 0, mid>;
		using Right = _franck_impl::ProductRange<Sample, n, mid + 1, n>;
		
		Sample x = fractional + latency;

		Left left(x);
		Right right(x);

		return left.calculateResult(right.total, data, invDivisors) + right.calculateResult(left.total, data, invDivisors);
	}
};

template<typename Sample, int n>
struct LagrangeBarycentric {
	static constexpr int inputLength = n + 1;
	static constexpr int latency = (n - 1)/2;

	std::array<Sample, (n + 1)> weights;
	
	LagrangeBarycentric() {
		for (int j = 0; j <= n; ++j) {
			double divisor = 1;
			for (int k = 0; k < j; ++k) divisor *= (j - k);
			for (int k = j + 1; k <= n; ++k) divisor *= (j - k);
			weights[j] = 1/divisor;
		}
	}
	
	template<class Data>
	Sample fractional(const Data &data, Sample fractional) const {
		if (fractional == 0 || fractional == 1) return data[latency + fractional];

		Sample x = fractional + latency;
		// We can special-case index 0
		Sample component0 = weights[0]/x;
		Sample componentTotal = component0;
		Sample resultTotal = component0*data[0];
		for (int i = 1; i <= n; ++i) {
			Sample component = weights[i]/(x - i);
			componentTotal += component;
			resultTotal += component*data[i];
		}
		return resultTotal/componentTotal;
	}
};

/*////// Testing logic //////*/

template<typename Sample, int n, class Interpolator>
static double measureInterpolator(Test &test, bool testAgainstReference) {
	using InterpolatorReference = signalsmith::delay::InterpolatorLagrangeN<Sample, n>;

	std::vector<Sample> buffer(1024);
	for (size_t i = 0; i < buffer.size(); ++i) {
		buffer[i] = test.random(-10, 10);
	}

	int testBlock = 100000;
	std::vector<Sample> delayTimes(testBlock);
	std::vector<Sample> result(testBlock);
	for (int i = 0; i < testBlock; ++i) {
		delayTimes[i] = test.random(0, 1);
	}
	
	Interpolator interpolator;
	if (testAgainstReference) {
		InterpolatorReference reference; // validated by other tests
		for (double d : delayTimes) {
			double expected = reference.fractional(buffer, d);
			double actual = interpolator.fractional(buffer, d);
			if (!test.closeEnough(actual, expected, "actual ~= expected", 1e-4)) {
				std::cout << actual << " != " << expected << "\n";
				return 0;
			}
		}
	}
	
	int trials = 100;
	Stopwatch stopwatch;
	for (int t = 0; t < trials; ++t) {
		stopwatch.startLap();
		for (int i = 0; i < testBlock; ++i) {
			result[i] = interpolator.fractional(buffer, delayTimes[i]);
		}
		stopwatch.lap();
	}
	double lapTime = stopwatch.optimistic();
	std::cout << "\t" << n << ":\t" << lapTime << "\n";
	return lapTime;
}

template<typename Sample, template<typename, int> class Interpolator>
std::vector<double> measureInterpolatorFamily(Test &test, std::string name, bool testAgainstReference) {
	std::cout << name << ":\n";
	std::vector<double> result;
#define PERF_TEST(n) \
	result.push_back(measureInterpolator<Sample, n, Interpolator<Sample, n>>(test, testAgainstReference));
	PERF_TEST(3)
	PERF_TEST(5)
	PERF_TEST(7)
	PERF_TEST(9)
	PERF_TEST(11)
	PERF_TEST(13)
	PERF_TEST(15)
	PERF_TEST(17)
	PERF_TEST(19)
	return result;
}

template<typename Sample, int n>
using KaiserSincN = signalsmith::delay::InterpolatorKaiserSincN<Sample, n>; // default minPhase argument

template<typename Sample>
struct PerformanceResults {
	Test &test;
	struct Pair {
		std::string name;
		std::vector<double> speeds;
	};
	std::vector<Pair> pairs;
	
	PerformanceResults(Test &test) : test(test) {}
	
	template<template<typename, int> class Interpolator>
	void add(std::string name, bool testAgainstReference=true) {
		if (!test.success) return;
		std::vector<double> speeds = measureInterpolatorFamily<Sample, Interpolator>(test, name, testAgainstReference);
		pairs.push_back({name, speeds});
	}
	
	void runAll(std::string csvName) {
		add<signalsmith::delay::InterpolatorLagrangeN>("current");
	//	add<LagrangeMulAddBasic>("mul/add basic");
		add<LagrangeMulAddFranck>("Franck");
		add<LagrangeBarycentric>("barycentric");
		add<KaiserSincN>("kaiser-sinc", false);

		CsvWriter csv(csvName);
		csv.write("N");
		for (auto &pair : pairs) csv.write(pair.name);
		csv.line();

		for (size_t i = 0; i < pairs[0].speeds.size(); ++i) {
			csv.write(3 + 2*i);
			for (auto &pair : pairs) csv.write(pair.speeds[i]);
			csv.line();
		}
	}
};

TEST("Performance: Lagrange interpolation (double)", performance_lagrange_double) {
	PerformanceResults<double> results(test);
	results.runAll("performance-lagrange-interpolation-double");
}
TEST("Performance: Lagrange interpolation (float)", performance_lagrange_float) {
	PerformanceResults<float> results(test);
	results.runAll("performance-lagrange-interpolation-float");
}
