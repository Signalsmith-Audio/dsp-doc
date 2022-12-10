#include "rates.h"
#include "fft.h"

#include <complex>
#include <cmath>
#include <vector>

// from the shared library
#include <test/tests.h>
#include "../common.h"

TEST("Oversampler2xFIR linearity") {
	int channels = 2, maxBlock = 1000;
	signalsmith::rates::Oversampler2xFIR<float> oversamplerA(channels, maxBlock), oversamplerB(channels, maxBlock), oversamplerSum(channels, maxBlock);
	
	std::vector<std::vector<double>> bufferA(channels), bufferB(channels), bufferSum(channels);
	for (int r = 0; r < 10; r++) {
		int blockLength = test.randomInt(0, maxBlock);
		for (int c = 0; c < channels; ++c) {
			bufferA[c].assign(blockLength, 0);
			bufferB[c].assign(blockLength, 0);
			bufferSum[c].assign(blockLength, 0);
			for (int i = 0; i < blockLength; ++i) {
				bufferA[c][i] = test.random(-10, 10);
				bufferB[c][i] = test.random(-10, 10);
				bufferSum[c][i] = bufferA[c][i] + bufferB[c][i];
			}
		}
		oversamplerA.up(bufferA, blockLength);
		oversamplerB.up(bufferB, blockLength);
		oversamplerSum.up(bufferSum, blockLength);
		
		double totalError2 = 0;
		for (int c = 0; c < channels; ++c) {
			for (int i = 0; i < blockLength; ++i) {
				double sum = bufferA[c][i] + bufferB[c][i];
				double error = bufferSum[c][i] - sum;
				totalError2 += error*error;
			}
		}
		double rmsError = std::sqrt(totalError2/channels/blockLength);
		TEST_APPROX(rmsError, 0, 1e-15);
	}
}

TEST("Oversampler2xFIR block boundaries") {
	std::vector<float> input(1000);
	for (auto &d : input) d = test.random(-2, 2);
	std::vector<float> outputA(1000), outputB(1000), outputC(1000);
	
	signalsmith::rates::Oversampler2xFIR<float> oversampler(2, 1000);
	
	for (int r = 0; r < 10; ++r) {
		int block1 = test.randomInt(0, 1000);
		int block2 = test.randomInt(0, 1000);
		if (block1 > block2) std::swap(block1, block2);
		
		oversampler.reset();
		oversampler.upChannel(0, input.data(), block1);
		oversampler.downChannel(0, outputA.data(), block1);
		oversampler.upChannel(0, input.data() + block1, block2 - block1);
		oversampler.downChannel(0, outputA.data() + block1, block2 - block1);
		oversampler.upChannel(0, input.data() + block2, 1000 - block2);
		oversampler.downChannel(0, outputA.data() + block2, 1000 - block2);

		// Second channel, should be independent
		// only split into two
		oversampler.upChannel(1, input.data(), block2);
		oversampler.downChannel(1, outputB.data(), block2);
		oversampler.upChannel(1, input.data() + block2, 1000 - block2);
		oversampler.downChannel(1, outputB.data() + block2, 1000 - block2);

		// Reset, and do the whole thing at once
		oversampler.reset();
		oversampler.upChannel(0, input, 1000);
		oversampler.downChannel(0, outputC, 1000);
		
		double totalError2 = 0;
		for (int i = 0; i < 1000; ++i) {
			double errorAB = outputB[i] - outputA[i];
			double errorAC = outputC[i] - outputA[i];
			totalError2 += errorAB*errorAB + errorAC*errorAC;
		}
		double rmsError = std::sqrt(totalError2/2000);
		TEST_APPROX(rmsError, 0, 1e-15);
	}
}

TEST("Oversampler2xFIR upsample") {
	int maxBlock = 1024;
	signalsmith::rates::Oversampler2xFIR<double> oversampler(2, maxBlock);

	signalsmith::fft::FFT<double> fftUp(maxBlock*2), fftDown(maxBlock);
	std::vector<std::complex<double>> spectrumUp(maxBlock*2), spectrumDown(maxBlock);

	auto plotResponses = [&](std::vector<int> halfLengths, double passFreq, std::string passLabel, std::string svgFile) {
		Figure figure;
		auto &upPlot = figure(0, 0).plot(350, 120);
		auto &downPlot = figure(0, 1).plot(350, 120);
		upPlot.x.linear(0, 1).major(0).minor(0.5).minor(1);
		upPlot.y.label("upsample response (dB)").linear(-150, 0).minors(0, -30, -60, -90, -120, -150);
		auto &passLabelAxis = upPlot.newX().flip().linear(0, 1).minor(passFreq, passLabel).minor(1 - passFreq, "");
		downPlot.x.copyFrom(upPlot.x).label("frequency");
		downPlot.y.copyFrom(upPlot.y).label("downsample response (dB)");
		downPlot.newX().copyFrom(passLabelAxis).blankLabels();
		auto &legend = upPlot.legend(0, 0);
		
		std::vector<double> buffer;
		for (auto halfLength : halfLengths) {
			buffer.assign(maxBlock, 0);
			// impulse somewhere in the first half
			buffer[test.randomInt(0, maxBlock/2)] = 1;
			
			oversampler.resize(1, maxBlock, halfLength, passFreq);
			oversampler.reset();
			oversampler.upChannel(0, buffer, maxBlock);
			fftUp.fft(oversampler[0], spectrumUp);

			auto &upLine = upPlot.line();
			legend.line(upLine, "L = " + std::to_string(halfLength) + "*2 = " + std::to_string(halfLength*2));
			for (int f = 0; f <= maxBlock; ++f) {
				double db = 10*std::log10(std::norm(spectrumUp[f])/4 + 1e-300);
				upLine.add(f*1.0/maxBlock, db);
			}
			
			for (int i = 0; i < maxBlock*2; ++i) oversampler[0][i] = 0;
			int offset = test.randomInt(0, maxBlock - halfLength*2);
			signalsmith::rates::fillKaiserSinc(oversampler[0] + offset, maxBlock, 0.25);
			for (int i = 0; i < maxBlock*2; ++i) {
				double sign = (i&1) ? -1 : 1;
				oversampler[1][i] = sign*oversampler[0][i];
			}

			auto &downLine = downPlot.line();
			// Lower half of spectrum (should be preserved, ideally)
			oversampler.downChannel(0, buffer, maxBlock);
			fftDown.fft(buffer, spectrumDown);
			for (int f = 0; f <= maxBlock/2; ++f) {
				double db = 10*std::log10(std::norm(spectrumDown[f])*4 + 1e-300);
				downLine.add(f*1.0/maxBlock, db);
			}
			oversampler.downChannel(1, buffer, maxBlock);
			fftDown.fft(buffer, spectrumDown);
			for (int f = maxBlock/2; f >= 0; --f) {
				double db = 10*std::log10(std::norm(spectrumDown[f])*4 + 1e-300);
				downLine.add(1 - f*1.0/maxBlock, db);
			}
		}
		
		figure.write(svgFile + "-both.svg");
		upPlot.y.label("up/down filter (dB)");
		upPlot.x.label("frequency");
		upPlot.write(svgFile + ".svg", figure.defaultStyle());
	};
	plotResponses({10, 20, 30, 40}, 0.45, "pass = 0.45", "rates-oversampler2xfir-responses-45");
}

TEST("Oversampler2xFIR tradeoffs") {
	int maxBlock = 2048;
	signalsmith::rates::Oversampler2xFIR<double> oversampler(1, maxBlock);
	std::vector<double> buffer(maxBlock);

	signalsmith::fft::FFT<double> fft(maxBlock*2);
	std::vector<std::complex<double>> spectrum(maxBlock*2);

	struct Stats {
		double peakDb, averageDb;
	};
	auto getStats = [&](int halfLength, double passFreq) {
		buffer.assign(maxBlock, 0);
		// impulse somewhere in the first half
		buffer[test.randomInt(0, maxBlock/2)] = 1;
			
		oversampler.resize(1, maxBlock, halfLength, passFreq);
		oversampler.reset();
		oversampler.upChannel(0, buffer, maxBlock);
		fft.fft(oversampler[0], spectrum);
		
		double peakDb = -100000;
		int stopIndex = std::ceil((1 - passFreq)*maxBlock);
		double energyCounter = 0, energySum = 0;
		for (int f = stopIndex; f < maxBlock; ++f) {
			double energy = std::norm(spectrum[f])/4;
			energySum += energy;
			energyCounter++;
			
			double db = 10*std::log10(energy + 1e-300);
			if (db > peakDb) peakDb = db;
		}
		return Stats{peakDb, 10*std::log10(energySum/energyCounter + 1e-300)};
	};

	Figure figure;
	int plotIndex = 0;
	auto plotPassFreq = [&](double passFreq, std::string title, int maxH, int hStep) {
		int plotX = (plotIndex%2), plotY = plotIndex/2;
		auto &plot = figure(plotX, plotY).plot(144, 120);
		plot.x.linear(0, maxH).major(0);
		for (int h = 0; h <= maxH; h += hStep) plot.x.minor(h);
		plot.y.linear(-150, 0).minors(0, -30, -60, -90, -120, -150);
		plot.newX().flip().label(title);
		
		auto &peakLine = plot.line();
		auto &averageLine = plot.line();
		if (plotX == 0) plot.y.label("error (dB)");
		if (plotY == 1) plot.x.label("half-width");
		if (plotX == 1 && plotY == 0) plot.legend(2, 1).line(peakLine, "peak").line(averageLine, "average");
		
		for (int h = 0; h <= maxH; ++h) {
			auto stats = getStats(h, passFreq);
			peakLine.add(h, stats.peakDb);
			averageLine.add(h, stats.averageDb);
		}
		++plotIndex;
	};
	plotPassFreq(0.25, "pass = 0.25", 10, 2);
	plotPassFreq(0.4, "pass = 0.4", 25, 5);
	plotPassFreq(0.45, "pass = 0.45", 50, 10);
	plotPassFreq(0.48, "pass = 0.48", 120, 20);
	figure.write("rates-oversampler2xfir-lengths.svg");
}
