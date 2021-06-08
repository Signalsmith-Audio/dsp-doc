#include "../common.h"

#include <complex>
#include <cmath>
#include <test/tests.h>

#include "spectral.h"
#include "fft.h"

void testKaiser(Test &test, const std::vector<int> &overlaps, const std::vector<double> &aliasingLimits, bool perfectReconstruction, bool heuristicOptimal) {
	int length = 256*3;
	
	int oversample = 64;
	signalsmith::RealFFT<double> realFft(length*oversample);
	
	std::vector<std::vector<double>> windows(overlaps.size());

	std::vector<double> timeBuffer;
	std::vector<std::complex<double>> spectrum;
	for (size_t i = 0; i < overlaps.size(); ++i) {
		int overlap = overlaps[i];
		windows[i].resize(length);

		auto kaiser = signalsmith::windows::Kaiser::withBandwidth(overlap, heuristicOptimal);
		kaiser.fill(windows[i], length);
		if (perfectReconstruction) {
			signalsmith::windows::forcePerfectReconstruction(windows[i], length, length/overlap);
		}
		
		timeBuffer = windows[i];
		timeBuffer.resize(length*oversample);
		spectrum.resize(length*oversample/2);
		realFft.fft(timeBuffer, spectrum);
		
		double aliasingLimit = aliasingLimits[i];
		int boundaryIndex = oversample*overlap/2;
		double peakDb = -300;
		double magRef = std::norm(spectrum[0]);
		for (int i = boundaryIndex; i < length*oversample/2; ++i) {
			double mag2 = std::norm(spectrum[i])/magRef;
			double db = 10*std::log10(mag2 + 1e-30);
			if (db > aliasingLimit) {
				std::cout << overlap << "x overlap at bin " << i << "/" << oversample << "\n";
				std::cout << db << " > " << aliasingLimit << "\n";
				return test.fail("Aliasing too high");
			}
			peakDb = std::max(peakDb, db);
		}
		std::cout << "\taliasing peak for x" << overlap << ": " << peakDb << " dB\n";
	}

	std::string name = "kaiser-windows";
	if (heuristicOptimal) name += "-heuristic";
	if (perfectReconstruction) name += "-pr";
	CsvWriter csv(name);
	csv.write("x");
	for (int overlap : overlaps) csv.write(overlap);
	csv.line();
	for (int i = 0; i < length; ++i) {
		csv.write(i*1.0/length);
		for (auto &window : windows) {
			csv.write(window[i]);
		}
		csv.line();
	}
}

TEST("Kaiser window", stft_kaiser_windows_plain) {
	std::vector<int> overlaps = {2, 4, 6, 8};
	std::vector<double> aliasingLimits = {-13, -39, -65, -91};

	testKaiser(test, overlaps, aliasingLimits, false, false);
}

TEST("Kaiser window (heuristic optimal)", stft_kaiser_windows) {
	std::vector<int> overlaps = {2, 4, 6, 8};
	std::vector<double> aliasingLimits = {-14, -41, -65.5, -91};

	testKaiser(test, overlaps, aliasingLimits, false, true);
}
TEST("Kaiser window (heuristic optimal P-R scaled)", stft_kaiser_windows_pr) {
	std::vector<int> overlaps = {2, 4, 6, 8};
	std::vector<double> aliasingLimits = {-14, -41, -65.5, -91};

	testKaiser(test, overlaps, aliasingLimits, true, true);
}
