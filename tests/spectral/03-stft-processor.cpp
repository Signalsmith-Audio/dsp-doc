#include "spectral.h"

#include <complex>
#include <array>
#include <limits>

#include <test/tests.h>

#include "../common.h"

TEST("STFT processor", stft_writer_sample_by_sample) {
	constexpr int channels = 3;
	constexpr int windowSize = 511;
	constexpr int interval = 256;

	int inputLength = 10.5*interval;
	signalsmith::delay::MultiBuffer<float> inputBuffer(channels, inputLength);
	signalsmith::delay::MultiBuffer<float> outputBuffer(channels, inputLength + windowSize);
	// Fill input with random data
	for (int c = 0; c < channels; ++c) {
		auto channel = inputBuffer[c];
		for (int i = 0; i < inputLength; ++i) {
			channel[i] = test.random(-1, 1);
		}
	}

	struct MySTFT : public signalsmith::spectral::ProcessSTFT<float> {
		using signalsmith::spectral::ProcessSTFT<float>::ProcessSTFT;
		
		int spectrumCount = 0;
		void processSpectrum(int) {
			++spectrumCount;
		}
	};
	MySTFT stft(channels, channels, windowSize, interval, inputLength + windowSize);

	 /* window size rounds up, then gets halved */
	TEST_ASSERT(stft.fftSize() == 512);
	TEST_ASSERT(stft.bands() == 256);
	TEST_ASSERT(stft.interval() == 256);

	for (int i = 0; i < inputLength; ++i) {
		for (int c = 0; c < channels; ++c) {
			stft.input[c][i] = inputBuffer[c][i];
		}
		stft.ensureValid(i);
		outputBuffer.at(i) = stft.at(i);
	}
	
	TEST_ASSERT(stft.spectrumCount >= 10);
	TEST_ASSERT(stft.spectrumCount <= 11);
	
	int latency = stft.latency();
	TEST_ASSERT(latency >= windowSize - 1 && latency <= windowSize + interval);
	// Input is passed through unchanged, with latency
	for (int i = latency + windowSize/* first blocks will be missing */; i < inputLength; ++i) {
		for (int c = 0; c < channels; ++c) {
			TEST_ASSERT(test.closeEnough(inputBuffer[c][i - latency], outputBuffer[c][i], "outputs match", 1e-4));
		}
	}
}
