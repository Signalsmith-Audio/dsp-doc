#include "spectral.h"

#include <complex>
#include <array>
#include <limits>

#include <test/tests.h>

#include "../common.h"

TEST("STFT writer sample-by-sample", stft_writer_sample_by_sample) {
	constexpr int channels = 2;
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

	signalsmith::spectral::STFT<float> stft(channels, windowSize, interval, inputLength + windowSize);

	 /* window size rounds up, then gets halved */
	TEST_ASSERT(stft.fftSize() == 512);
	TEST_ASSERT(stft.bands() == 256);

	int spectrumCount = 0;
	for (int i = 0; i < inputLength; ++i) {
		stft.ensureValid(i, [&](int i) {
			stft.analyse(inputBuffer.view(i));
			spectrumCount++;
		});
		outputBuffer.at(i) = stft.at(i);
	}
	
	TEST_ASSERT(spectrumCount >= 10);
	TEST_ASSERT(spectrumCount <= 11);
	
	int latency = stft.latency();
	TEST_ASSERT(latency >= 0 && latency <= interval);
	// Input is passed through unchanged, with latency
	for (int i = latency + windowSize/* first blocks will be missing */; i < inputLength; ++i) {
		for (int c = 0; c < channels; ++c) {
			TEST_ASSERT(test.closeEnough(inputBuffer[c][i - latency], outputBuffer[c][i], "outputs match", 1e-4));
		}
	}
}

TEST("STFT window sanity-check", stft_window_checks) {
	signalsmith::spectral::STFT<double> stft;
	
	CsvWriter windowCsv("stft-windows");
	CsvWriter partialCsv("stft-windows-partial");

	int windowSize = 250;
	int intervals[] = {191, 130, 96, 55, 32};
	constexpr int intervalCount = sizeof(intervals)/sizeof(intervals[0]);

	windowCsv.write("index");
	partialCsv.write("index");
	for (int i = 0; i < 5; ++i) {
		windowCsv.write(intervals[i]);
		partialCsv.write(intervals[i]);
	}
	windowCsv.line();
	partialCsv.line();
	
	std::vector<std::vector<double>> windows, partialWindows;
	for (int i = 0; i < intervalCount; ++i) {
		int interval = intervals[i];
		stft.resize(1, windowSize, interval);
		
		windows.push_back(stft.window());
		partialWindows.push_back(stft.partialSumWindow());
	}
	for (int i = 0; i < windowSize; ++i) {
		windowCsv.write(i);
		partialCsv.write(i);
		for (int j = 0; j < intervalCount; ++j) {
			windowCsv.write(windows[j][i]);
			partialCsv.write(partialWindows[j][i]);
			
			constexpr double fudge = 1e-4;

//			if (i > 0 && i < windowSize/2) {
//				TEST_ASSERT(windows[j][i] >= windows[j][i - 1] - fudge); // Monotonically increasing in first half
//			}
//			if (i < windowSize - 1 && i > windowSize/2) {
//				TEST_ASSERT(windows[j][i] >= windows[j][i + 1] - fudge); // Monotonically in first half
//			}
			if (i > 0) {
				TEST_ASSERT(partialWindows[j][i] <= partialWindows[j][i - 1] + fudge); // Monotonically decreasing the whole way
			}
		}
		windowCsv.line();
		partialCsv.line();
	}
}

TEST("STFT analyse() and analyseRaw()", stft_analyse) {
	signalsmith::spectral::STFT<double> stft(2, 256, 128);
	
	signalsmith::ModifiedRealFFT<double> fft(256);
	
	std::vector<std::vector<double>> input(2);
	for (int c = 0; c < 2; ++c) {
		auto &channel = input[c];
		channel.resize(256);
		for (int i = 0; i < 256; ++i) {
			channel[i] = test.random(-1, 1);
		}
	}

	// Un-windowed analysis
	stft.analyseRaw(input);

	std::vector<std::complex<double>> spectrum(256);
	for (int c = 0; c < 2; ++c) {
		fft.fft(input[c], spectrum);
		for (int f = 0; f < 128; f++) {
			TEST_ASSERT(stft.spectrum[c][f] == spectrum[f])
		}
	}

	// Windowed analysis
	stft.analyse(input);

	// Apply the window to the input
	auto &window = stft.window();
	for (int c = 0; c < 2; ++c) {
		for (int i = 0; i < 256; i++) {
			input[c][i] *= window[i];
		}
		fft.fft(input[c], spectrum);
		for (int f = 0; f < 128; f++) {
			TEST_ASSERT(stft.spectrum[c][f] == spectrum[f])
		}
	}
}

TEST("STFT aliasing check", stft_aliasing) {
	constexpr int channels = 1;
	
	signalsmith::spectral::STFT<double> stft;
	std::vector<double> ones;
	
	auto aliasingForOverlap = [&](int windowSize, int overlap) {
		ones.assign(windowSize + 10, 1);
		// Break if we do any calculations with this ;)
		for (int i = windowSize; i < (int)ones.size(); ++i) {
			ones[i] = 0;
//			ones[i] = std::numeric_limits<double>::signaling_NaN();
		}

		int inputLength = overlap*10;
		stft.resize(channels, windowSize, overlap, inputLength);
		stft += (int)test.random(0, inputLength*2);
		stft += windowSize; // Enough for the output to warm up

		// filter made from random phase shifts
		std::vector<std::complex<double>> filter(stft.bands());
		for (int f = 0; f < stft.bands(); ++f) {
			double phase = test.random(-M_PI, M_PI);
			filter[f] = {std::cos(phase), std::sin(phase)};
		}
		// Fill the output
		stft.ensureValid(inputLength - 1, [&](int){
			stft.analyse(&ones);
			for (int f = 0; f < stft.bands(); ++f) {
				stft.spectrum[0][f] *= filter[f];
			}
		});
		
		auto channel0 = stft[0];
		double sum = 0, sum2 = 0;
		for (int i = 0; i < inputLength; ++i) {
			double v = channel0[i];
			sum += v;
			sum2 += v*v;
		}
		// Original signal was constant 1s - so any variance is aliasing
		double mean = sum/inputLength;
		double variance = sum2/inputLength - mean*mean;
		return variance;
	};
	
	auto averageAliasing = [&](int windowSize, int overlap) {
		int repeats = 10;
		double sum2 = 0;
		for (int r = 0; r < repeats; ++r) {
			sum2 += aliasingForOverlap(windowSize, overlap);
		}
		return 10*std::log10(sum2/repeats);
	};

	std::array<int, 4> windowSizes = {70, 128, 163, 257};
	for (int windowSize : windowSizes) {
		std::cout << "\twindow size = " << windowSize << "\n";
		CsvWriter csv("stft-aliasing-simulated-" + std::to_string(windowSize));
		csv.line("window", "overlap", "aliasing");
		for (int overlap = windowSize; overlap >= windowSize/12; --overlap) {
			double ratio = windowSize*1.0/overlap;
			double failLimitDb = 17.5 - 14.5*ratio; // an eyeballed performance limit, as a smoke test
			
			double db = averageAliasing(windowSize, overlap);
			csv.line(windowSize, overlap, db);
			
			if (ratio <= 10) {
				TEST_ASSERT(db < failLimitDb);
			}
		}
	}
}

