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
				test.log(overlap, "x overlap at bin ", i, "/", oversample);
				test.log(db, " > ", aliasingLimit);
				return test.fail("Aliasing too high");
			}
			peakDb = std::max(peakDb, db);
		}
		test.log("aliasing peak for x", overlap, ": ", peakDb, " dB");
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
	std::vector<double> aliasingLimits = {-13, -41, -66, -92};

	testKaiser(test, overlaps, aliasingLimits, false, true);
}
TEST("Kaiser window (heuristic optimal P-R scaled)", stft_kaiser_windows_pr) {
	std::vector<int> overlaps = {2, 4, 6, 8};
	std::vector<double> aliasingLimits = {-13, -41, -66, -92};

	testKaiser(test, overlaps, aliasingLimits, true, true);
}

struct SidelobeStats {
	double mainPeak = 0, sidePeak = 0;
	double mainEnergy = 0, sideEnergy = 0;
};
SidelobeStats measureKaiser(double bandwidth, double measureBandwidth, bool forcePR=false) {
	SidelobeStats result;

	int length = 256;
	int oversample = 64;
	signalsmith::RealFFT<double> realFft(length*oversample);
	std::vector<double> window(length*oversample, 0);
	
	auto kaiser = signalsmith::windows::Kaiser::withBandwidth(bandwidth);
	kaiser.fill(window, length); // Leave the rest as zero padding (oversamples the frequency domain)
	if (forcePR) {
		signalsmith::windows::forcePerfectReconstruction(window, length, length/measureBandwidth);
	}

	std::vector<std::complex<double>> spectrum(window.size()/2);
	realFft.fft(window, spectrum);
	
	for (size_t b = 0; b < spectrum.size(); ++b) {
		double freq = b*1.0/oversample;
		double energy = std::norm(spectrum[b]);
		double abs = std::sqrt(energy);
		if (freq <= measureBandwidth*0.5) {
			result.mainPeak = std::max(abs, result.mainPeak);
			result.mainEnergy += energy;
		} else {
			result.sidePeak = std::max(abs, result.sidePeak);
			result.sideEnergy += energy;
		}
	}
	return result;
}

TEST("Kaiser: beta & bandwidth", stft_kaiser_beta_bandwidth) {
	using Kaiser = signalsmith::windows::Kaiser;
	
	int points = 389;
	std::vector<double> dataA(points), dataB(points);
	
	for (double bw = 2; bw < 16; bw += 0.1) {
		double beta = Kaiser::bandwidthToBeta(bw);
		
		// Matches direct construction
		Kaiser(beta).fill(dataA, points);
		Kaiser::withBandwidth(bw).fill(dataB, points);
		for (int i = 0; i < points; ++i) {
			TEST_ASSERT(dataA[i] == dataB[i]);
		}
		
		double bw2 = Kaiser::betaToBandwidth(beta);
		if (!(std::abs(bw2 - bw) < 1e-6)) {
			return test.fail("bandwidths don't match");
		}
	}
}

double ampToDb(double energy) {
	return 20*std::log10(energy + 1e-100);
}
double energyToDb(double energy) {
	return 10*std::log10(energy + 1e-100);
}

TEST("Kaiser: bandwidth & sidelobes", stft_kaiser_bandwidth_sidelobes) {
	using Kaiser = signalsmith::windows::Kaiser;

	CsvWriter csv("kaiser-bandwidth-sidelobes");
	csv.line("bandwidth", "exact peak (dB)", "exact energy (dB)", "heuristic bandwidth", "heuristic peak (dB)", "heuristiic energy (dB)");

	auto optimalEnergyBandwidth = [&](double b) {
		double hb = Kaiser::betaToBandwidth(Kaiser::bandwidthToBeta(b, true));
		//hb = b + 8/((b + 3)*(b + 3)) + 0.25*std::max(3 - b, 0.0);

		// Brute-force search, used to tune the heuristic
		/*
		double increment = b*0.005;
		auto energyRatio = [&](double hb) {
			auto stats = measureKaiser(hb, b, true);
			double peakRatio = stats.sidePeak/(stats.mainPeak + 1e-100);
			double energyRatio = stats.sideEnergy/(stats.mainEnergy + 1e-100);
			return energyRatio + peakRatio*0.1/b;
		};

		double best = energyToDb(energyRatio(hb));
		for (double c = std::max<double>(b - 1, 0); c < b + 1; c += increment) {
			double candidate = energyToDb(energyRatio(c));
			if (candidate < best) {
				hb = c;
				best = candidate;
			}
		}
		//*/
		return hb;
	};

	for (double b = 0.1; b < 10; b += 0.1) {
		auto stats = measureKaiser(b, b);

		double peakRatio = stats.sidePeak/(stats.mainPeak + 1e-100);
		double energyRatio = stats.sideEnergy/(stats.mainEnergy + 1e-100);
		
		double energyDb = energyToDb(energyRatio);
		{ // Approximate energy ratio
			double predictedEnergyDb = Kaiser::bandwidthToEnergyDb(b);
			if (b >= 2 && b <= 10 && std::abs(energyDb - predictedEnergyDb) > 0.5) {
				test.log(b, ": ", energyDb, " ~= ", predictedEnergyDb);
				return test.fail("Energy approximation should be accurate within 0.5dB, within 2-10x range");
			}
			double predictedBandwidth = Kaiser::energyDbToBandwidth(predictedEnergyDb);
			if (std::abs(b - predictedBandwidth) > 1e-3) {
				test.log(b, " != ", predictedBandwidth, " @ ", predictedEnergyDb);
				return test.fail("inverse of predicted bandwidth (energy)");
			}
		}

		double peakDb = ampToDb(peakRatio);
		{ // Approximate peak ratio
			double predictedPeakDb = Kaiser::bandwidthToPeakDb(b);
			if (b >= 2 && b <= 9 && std::abs(peakDb - predictedPeakDb) > 0.5) {
				test.log(b, ": ", peakDb, " ~= ", predictedPeakDb);
				return test.fail("Peak approximation should be accurate within 0.5dB, within 2-9x range");
			}
			double predictedBandwidth = Kaiser::peakDbToBandwidth(predictedPeakDb);
			if (std::abs(b - predictedBandwidth) > 1e-3) {
				test.log(b, " != ", predictedBandwidth, " @ ", predictedPeakDb);
				return test.fail("inverse of predicted bandwidth (peak)");
			}
		}

		double hb = optimalEnergyBandwidth(b);
		auto hStats = measureKaiser(hb, b);
		double hPeakRatio = hStats.sidePeak/(hStats.mainPeak + 1e-100);
		double hEnergyRatio = hStats.sideEnergy/(hStats.mainEnergy + 1e-100);
		double hEnergyDb = energyToDb(hEnergyRatio);
		{ // Approximate energy ratio
			double predictedEnergyDb = Kaiser::bandwidthToEnergyDb(b, true);
			if (b >= 0.5 && b <= 10 && std::abs(hEnergyDb - predictedEnergyDb) > 0.5) {
				test.log(b, ": ", hEnergyDb, " ~= ", predictedEnergyDb);
				return test.fail("Heuristic energy approximation should be accurate within 0.5dB, within 0.5-10x range");
			}
			double predictedBandwidth = Kaiser::energyDbToBandwidth(predictedEnergyDb, true);
			if (std::abs(b - predictedBandwidth) > 1e-3) {
				test.log(b, " != ", predictedBandwidth, " @ ", predictedEnergyDb);
				return test.fail("inverse of predicted bandwidth (heuristic energy)");
			}
		}
		double hPeakDb = ampToDb(hPeakRatio);
		{ // Approximate peak ratio
			double predictedPeakDb = Kaiser::bandwidthToPeakDb(b, true);
			if (b >= 0.5 && b <= 9 && std::abs(hPeakDb - predictedPeakDb) > 0.5) {
				test.log(b, ": ", hPeakDb, " ~= ", predictedPeakDb);
				return test.fail("Heuristic peak approximation should be accurate within 0.5dB, within 0.5-9x range");
			}
			double predictedBandwidth = Kaiser::peakDbToBandwidth(predictedPeakDb, true);
			if (b > 0.5 && std::abs(b - predictedBandwidth) > 1e-3) {
				test.log(b, " != ", predictedBandwidth, " @ ", predictedPeakDb);
				return test.fail("inverse of predicted bandwidth (heuristic peak)");
			}
		}

		csv.line(b, ampToDb(peakRatio), energyDb, hb, ampToDb(hPeakRatio), energyToDb(hEnergyRatio));
	}
}
