// from the shared library
#include <test/tests.h>

#include "../common.h"
#include "dsp/mix.h"

#include <complex>
#include <array>
#include <cmath>

template<typename Sample, size_t size>
void testHadamardTyped(Test test) {
	using Hadamard = signalsmith::mix::Hadamard<Sample, size>;
	for (size_t i = 0; i < size; ++i) {
		std::array<Sample, size> input, unscaled, scaled;
		for (size_t j = 0; j < size; ++j) {
			input[j] = (j == i);
		}
		unscaled = scaled = input;
		Hadamard::unscaledInPlace(unscaled);
		Hadamard::inPlace(scaled);
		for (size_t j = 0; j < size; ++j) {
			// Unscaled produces ±1
			TEST_EQUAL(std::abs(unscaled[j]), Sample(1));
			// Scaled is same, but scaled
			TEST_EQUAL(scaled[j], unscaled[j]*Hadamard::scalingFactor());
		}
	}

	// Linearity
	std::array<Sample, size> randomA, randomB, randomC, randomAB;
	Sample energyA = 0;
	for (size_t j = 0; j < size; ++j) {
		randomA[j] = test.random(-10, 10);
		randomB[j] = test.random(-10, 10);
		randomC[j] = test.random(-10, 10);
		randomAB[j] = randomA[j] + randomB[j];
		energyA += randomA[j]*randomA[j];
	}
	std::array<Sample, size> randomACopy = randomA;
	Hadamard::inPlace(randomA);
	Hadamard::inPlace(randomB.data());
	Hadamard::inPlace(randomAB);
	
	Sample energyOutA = 0;
	double accuracy = 0.0001;
	for (size_t j = 0; j < size; ++j) {
		TEST_APPROX(randomA[j] + randomB[j], randomAB[j], accuracy);
		energyOutA += randomA[j]*randomA[j];
	}
	TEST_APPROX(energyA, energyOutA, 0.01);
	
	// Dynamic size
	const signalsmith::mix::Hadamard<Sample> hadamard(size);
	TEST_EQUAL(hadamard.scalingFactor(), Hadamard::scalingFactor());
	hadamard.inPlace(randomACopy);
	for (size_t j = 0; j < size; ++j) {
		TEST_APPROX(randomACopy[j], randomA[j], 1e-5);
	}
}

template<size_t size>
void testHadamard(Test test) {
	testHadamardTyped<double, size>(test.prefix("double"));
	testHadamardTyped<float, size>(test.prefix("float"));
}

TEST("Hadamard") {
	testHadamard<0>(test.prefix("0"));
	testHadamard<1>(test.prefix("1"));
	testHadamard<2>(test.prefix("2"));
	testHadamard<4>(test.prefix("4"));
	testHadamard<8>(test.prefix("8"));
	testHadamard<16>(test.prefix("16"));
	testHadamard<32>(test.prefix("32"));
}

template<typename Sample, size_t size>
void testHouseholderTyped(Test test) {
	using Householder = signalsmith::mix::Householder<Sample, size>;
	TEST_EQUAL(Householder::scalingFactor(), Sample(1));

	// Linearity
	std::array<Sample, size> randomA, randomB, randomAB;
	Sample energyA = 0;
	for (size_t j = 0; j < size; ++j) {
		randomA[j] = test.random(-10, 10);
		randomB[j] = test.random(-10, 10);
		randomAB[j] = randomA[j] + randomB[j];
		energyA += randomA[j]*randomA[j];
	}
	std::array<Sample, size> randomACopy = randomA;
	Householder::inPlace(randomA);
	Householder::inPlace(randomB.data());
	Householder::inPlace(randomAB);
		
	Sample energyOutA = 0;
	double accuracy = 0.0001;
	for (size_t j = 0; j < size; ++j) {
		TEST_APPROX(randomA[j] + randomB[j], randomAB[j], accuracy);
		energyOutA += randomA[j]*randomA[j];
	}
	TEST_APPROX(energyA, energyOutA, 0.01);

	// Dynamic size
	const signalsmith::mix::Householder<Sample> householder(size);
	TEST_EQUAL(householder.scalingFactor(), Householder::scalingFactor());
	householder.inPlace(randomACopy);
	for (size_t j = 0; j < size; ++j) {
		TEST_APPROX(randomACopy[j], randomA[j], 1e-5);
	}
}

template<size_t size>
void testHouseholder(Test test) {
	testHouseholderTyped<double, size>(test.prefix("double"));
	testHouseholderTyped<float, size>(test.prefix("float"));
	testHouseholderTyped<std::complex<double>, size>(test.prefix("std::complex<double>"));
	testHouseholderTyped<std::complex<float>, size>(test.prefix("std::complex<float>"));
}

TEST("Householder") {
	testHouseholder<0>(test.prefix("0"));
	testHouseholder<1>(test.prefix("1"));
	testHouseholder<2>(test.prefix("2"));
	testHouseholder<3>(test.prefix("3"));
	testHouseholder<4>(test.prefix("4"));
	testHouseholder<5>(test.prefix("5"));
	testHouseholder<6>(test.prefix("6"));
}
