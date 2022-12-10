#include "rates.h"
#include "fft.h"

#include <complex>
#include <cmath>
#include <vector>

// from the shared library
#include <test/tests.h>
#include "../common.h"

TEST("Kaiser-sinc pass/stop specification") {
	int length = 33;
	int padTo = 2048;
	
	std::vector<double> buffer;
	
	signalsmith::fft::FFT<double> fft(padTo);
	std::vector<std::complex<double>> spectrum(padTo);
	
	Figure figure;
	auto &timePlot = figure(0, 0).plot(350, 125);
	auto &freqPlot = figure(0, 1).plot(350, 135);
	timePlot.x.linear(0, length - 1).label("index").minors(length - 1).major((length - 1)/2);
	timePlot.y.linear(-0.2, 1).label("signal").major(0).minor(1);
	freqPlot.x.linear(0, 0.5).label("frequency").major(0).minors(0.1, 0.2, 0.3, 0.4, 0.5);
	freqPlot.y.linear(-160, 10).label("dB").minors(-150, -120, -90, -60, -30, 0);
	auto &legend = timePlot.legend(1, 1);
	auto plotKernel = [&](double passFreq, double stopFreq) {
		char label[256];
		std::snprintf(label, 256, "%.1f to %.1f", passFreq, stopFreq);
		auto &timeLine = timePlot.line();
		auto &freqLine = freqPlot.line();
		legend.line(timeLine, label);
		
		buffer.resize(length);
		signalsmith::rates::fillKaiserSinc(buffer, length, passFreq, stopFreq);
		for (int i = 0; i < length; ++i) {
			timeLine.add(i, buffer[i]);
		}
		
		buffer.resize(padTo);
		fft.fft(buffer, spectrum);
		for (int f = 0; f <= padTo/2; ++f) {
			double scaledF = f*1.0/padTo;
			double energy = std::norm(spectrum[f]);
			double db = 10*std::log10(energy + 1e-300);
			freqLine.add(scaledF, db);
		}
	};
	plotKernel(0.1, 0.2);
	plotKernel(0.1, 0.3);
	plotKernel(0.1, 0.4);
	//plotKernel(0.4, 0.5);
	
	figure.write("rates-kaiser-sinc.svg");
	return test.pass();
}

TEST("Kaiser-sinc centre specification") {
	std::vector<int> lengths = {10, 30, 95};
	int padTo = 2048;
	double centreFreq = 0.25;
	
	std::vector<double> buffer;
	
	signalsmith::fft::FFT<double> fft(padTo);
	std::vector<std::complex<double>> spectrum(padTo);
	
	Figure figure;
	auto &freqPlot = figure.plot(350, 150);
	freqPlot.x.linear(0, 0.5).label("frequency").major(0).minor(0.5).minor(centreFreq);
	freqPlot.y.linear(-150, 10).label("dB").minors(-150, -120, -90, -60, -30, 0);
	auto &legend = freqPlot.legend(0, 0);
	for (auto &length : lengths) {
		char label[256];
		std::snprintf(label, 256, "N = %i", length);
		auto &freqLine = freqPlot.line();
		legend.add(freqLine.styleIndex, label, true, true);
		auto &fillLine = freqPlot.fill(freqLine.styleIndex);
		
		buffer.resize(length);
		signalsmith::rates::fillKaiserSinc(buffer, length, centreFreq);
		
		buffer.resize(padTo);
		fft.fft(buffer, spectrum);
		double halfWidth = 0.45/std::sqrt(length);
		fillLine.add(centreFreq - halfWidth, -300);
		for (int f = 0; f <= padTo/2; ++f) {
			double scaledF = f*1.0/padTo;
			double energy = std::norm(spectrum[f]);
			double db = 10*std::log10(energy + 1e-300);
			freqLine.add(scaledF, db);
			if (scaledF >= centreFreq - halfWidth && scaledF <= centreFreq + halfWidth) {
				fillLine.add(scaledF, db);
			}
		}
		fillLine.add(centreFreq + halfWidth, -300);
	}
	
	figure.write("rates-kaiser-sinc-heuristic.svg");
	return test.pass();
}
