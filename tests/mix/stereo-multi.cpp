// from the shared library
#include <test/tests.h>

#include "../common.h"
#include "dsp/mix.h"

#include <array>
#include <cmath>

template<typename Sample, int size>
void testStereoTyped(Test test) {
	const signalsmith::mix::StereoMultiMixer<Sample, size> mixer;

	std::array<Sample, 2> stereo{
		Sample(test.random(-10, 10)),
		Sample(test.random(-10, 10))
	};
	std::array<Sample, 2> stereoCopy = stereo;
	Sample inputEnergy = stereo[0]*stereo[0] + stereo[1]*stereo[1];
	std::array<Sample, size> multi;
	
	mixer.stereoToMulti(stereo, multi);
	// Upmix preserves energy for pairs
	for (int i = 0; i < size; i += 2) {
		Sample energy = multi[i]*multi[i] + multi[i + 1]*multi[i + 1];
		TEST_APPROX(energy, inputEnergy, 0.0001);
	}
	mixer.multiToStereo(multi, stereo);
	// When the results are in-phase, we should scale by `.scalingFactor1()`
	TEST_APPROX(stereo[0]*mixer.scalingFactor1(), stereoCopy[0], 0.0001);
	TEST_APPROX(stereo[1]*mixer.scalingFactor1(), stereoCopy[1], 0.0001);
	
	// Downmix preserves energy from pairs
	for (int pairStart = 0; pairStart < size; pairStart += 2) {
		for (auto &s : multi) s = 0;
		multi[pairStart] = test.random(-10, 10);
		multi[pairStart + 1] = test.random(-10, 10);
		Sample inputEnergy = multi[pairStart]*multi[pairStart] + multi[pairStart + 1]*multi[pairStart + 1];

		mixer.multiToStereo(multi, stereo);
		Sample outputEnergy = stereo[0]*stereo[0] + stereo[1]*stereo[1];
		TEST_APPROX(outputEnergy, inputEnergy, 0.0001);
	}
}

template<int size>
void testStereo(Test test) {
	testStereoTyped<double, size>(test.prefix("double"));
	testStereoTyped<float, size>(test.prefix("float"));
}

TEST("StereoMultiMix") {
	testStereo<2>(test.prefix("2"));
	testStereo<4>(test.prefix("4"));
	testStereo<6>(test.prefix("6"));
	testStereo<8>(test.prefix("8"));
	testStereo<10>(test.prefix("10"));
	testStereo<20>(test.prefix("20"));
}
