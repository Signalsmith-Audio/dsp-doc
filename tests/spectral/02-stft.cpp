#include "spectral.h"

#include <complex>
#include <array>
#include <limits>

#include <test/tests.h>

#include "../common.h"

TEST("STFT writer sample-by-sample") {
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
	TEST_ASSERT(stft.interval() == 256);

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

TEST("STFT window sanity-check") {
	using STFT = signalsmith::spectral::STFT<double>;
	STFT stft;
	
	auto testShape = [&](STFT::Window shape, std::string suffix) {
		stft.windowShape = shape;
	
		Figure figure;
		auto &singlePlot = figure(0, 0).plot(400, 130);
		auto &sumPlot = figure(0, 1).plot(400, 130);
		auto writeLater = figure.writeLater("stft-windows" + suffix + ".svg");
		singlePlot.y.linear(0, 1).major(0).tick(1).label("synthesis/analysis window");
		singlePlot.x.linear(0, 1).minors(0, 0.5 ,1);
		sumPlot.y.copyFrom(singlePlot.y).label("windowed cumulative sum");
		sumPlot.x.copyFrom(singlePlot.x);
		auto &legend = sumPlot.legend(0, 0);

		int windowSize = 250;
		int intervals[] = {191, 130, 96, 55, 32};
		constexpr int intervalCount = sizeof(intervals)/sizeof(intervals[0]);

		std::vector<std::vector<double>> windows, partialWindows, partialWindowsNext;
		for (int i = 0; i < intervalCount; ++i) {
			int interval = intervals[i];
			stft.resize(1, windowSize, interval);
			
			auto &windowLine = singlePlot.line();
			windowLine.add(0, 0);
			auto &sumLine = sumPlot.line();
			char lineLabel[256];
			std::snprintf(lineLabel, 256, "%.1fx (%i/%i)", windowSize*1.0/interval, interval, windowSize);
			legend.add(windowLine, lineLabel);
			
			auto &&window = stft.window();
			auto &&sumWindow = stft.partialSumWindow();
			for (int i = 0; i < windowSize; ++i) {
				windowLine.add(i/(windowSize - 1.0), window[i]);
				sumLine.add(i/(windowSize - 1.0), sumWindow[i]);
			}
			windowLine.add(1, 0);
			sumLine.add(1, 0);
			
			windows.push_back(stft.window());
			partialWindows.push_back(stft.partialSumWindow());
			partialWindowsNext.push_back(stft.partialSumWindow(false));
		}
		for (int i = 0; i < windowSize; ++i) {
			for (int j = 0; j < intervalCount; ++j) {
				constexpr double fudge = 1e-4;
				if (i > 0) {
					TEST_ASSERT(partialWindows[j][i] <= partialWindows[j][i - 1] + fudge); // Monotonically decreasing the whole way
				}
				double diff = partialWindows[j][i] - windows[j][i]*windows[j][i] - partialWindowsNext[j][i];
				TEST_ASSERT(std::abs(diff) < fudge); // passing `false` omits the first window
			}
		}
	};
	
	testShape(STFT::Window::kaiser, "");
	testShape(STFT::Window::acg, "-acg");
}

TEST("STFT analyse() and analyseRaw()") {
	signalsmith::spectral::STFT<double> stft(2, 256, 128);
	
	signalsmith::fft::ModifiedRealFFT<double> fft(256);
	
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

TEST("STFT aliasing check") {
	constexpr int channels = 1;
	
	using STFT = signalsmith::spectral::STFT<double>;
	STFT stft;
	std::vector<double> ones;
	
	auto aliasingForOverlap = [&](int windowSize, int overlap, STFT::Window windowShape) {
		ones.assign(windowSize + 10, 1);
		// Break if we do any calculations with this ;)
		for (int i = windowSize; i < (int)ones.size(); ++i) {
			ones[i] = 0;
//			ones[i] = std::numeric_limits<double>::signaling_NaN();
		}

		int inputLength = overlap*10;
		stft.windowShape = windowShape;
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
	
	auto averageAliasing = [&](int windowSize, int overlap, STFT::Window windowShape) {
		int repeats = 10;
		double sum2 = 0;
		for (int r = 0; r < repeats; ++r) {
			sum2 += aliasingForOverlap(windowSize, overlap, windowShape);
		}
		return 10*std::log10(sum2/repeats);
	};

	auto plotAliasing = [&](std::string svgFile, bool acg) {
		STFT::Window windowShape = acg ? STFT::Window::acg : STFT::Window::kaiser;
	
		std::array<int, 5> windowSizes = {70, 128, 163, 257, 32*9};
		Plot2D plot(250, 165);
		plot.x.linear(1, 12).major(1, "").minors(2, 4, 6, 8, 10, 12).label("overlap ratio (window/interval)");
		plot.y.linear(-150, 0).major(-150, "").minors(-140, -120, -100, -80, -60, -40, -20, 0).label("aliasing (dB)");
		auto &legend = plot.legend(1, 1);
		for (int windowSize : windowSizes) {
			std::cout << "\twindow size = " << windowSize << "\n";
			auto &line = plot.line();
			legend.add(line, "N=" + std::to_string(windowSize));
			for (int overlap = windowSize; overlap >= windowSize/12; --overlap) {
				double ratio = windowSize*1.0/overlap;
				
				double db = averageAliasing(windowSize, overlap, windowShape);
				line.add(ratio, db);

				if (!acg) {
					double failLimitDb = 17.5 - 14.5*ratio; // an eyeballed performance limit, as a smoke test
					if (ratio <= 10) {
						TEST_ASSERT(db < failLimitDb);
					}
				}
			}
		}
		plot.write(svgFile);
	};
	plotAliasing("stft-aliasing-simulated.svg", false);
	plotAliasing("stft-aliasing-simulated-acg.svg", true);
}

