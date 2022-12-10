#include "../common.h"

#include <complex>
#include <cmath>
#include <test/tests.h>

#include "spectral.h"
#include "fft.h"

void testKaiser(Test &test, const std::vector<int> &overlaps, const std::vector<double> &aliasingLimits, bool perfectReconstruction, bool heuristicOptimal) {
	int length = 256*3;
	
	int oversample = 64;
	signalsmith::fft::RealFFT<double> realFft(length*oversample);
	
	std::vector<std::vector<double>> windows(overlaps.size());

	std::vector<double> timeBuffer;
	std::vector<std::complex<double>> spectrum;
	for (size_t i = 0; i < overlaps.size(); ++i) {
		int overlap = overlaps[i];
		windows[i].resize(length);

		auto kaiser = signalsmith::windows::Kaiser::withBandwidth(overlap, heuristicOptimal);
		kaiser.fill(windows[i], length);
		for (int s = 0; s < length; ++s) {
			double r = (s + 0.5)/length;
			TEST_ASSERT(std::abs(windows[i][s] - kaiser(r)) < 1e-10);
		}
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

TEST("Kaiser window") {
	std::vector<int> overlaps = {2, 4, 6, 8};
	std::vector<double> aliasingLimits = {-13, -39, -65, -91};

	testKaiser(test, overlaps, aliasingLimits, false, false);
}

TEST("Kaiser window (heuristic optimal)") {
	std::vector<int> overlaps = {2, 4, 6, 8};
	std::vector<double> aliasingLimits = {-13, -41, -66, -92};

	testKaiser(test, overlaps, aliasingLimits, false, true);

	CsvWriter csv("kaiser-stats");
	csv.line("bandwidth", "beta", "peak-dB", "energy-dB", "beta-optimal", "peak-dB", "energy-dB");
	using Kaiser = signalsmith::windows::Kaiser;
	for (double b = 1; b < 10; b += 0.5) {
		csv.line(b, Kaiser::bandwidthToBeta(b, false), Kaiser::bandwidthToPeakDb(b, false), Kaiser::bandwidthToEnergyDb(b, false), Kaiser::bandwidthToBeta(b, true), Kaiser::bandwidthToPeakDb(b, true), Kaiser::bandwidthToEnergyDb(b, true));
	}
}
TEST("Kaiser window (heuristic optimal P-R scaled)") {
	std::vector<int> overlaps = {2, 4, 6, 8};
	std::vector<double> aliasingLimits = {-13, -41, -66, -92};

	testKaiser(test, overlaps, aliasingLimits, true, true);
}

#include "./window-stats.h"

TEST("Kaiser: beta & bandwidth") {
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

double optimalKaiserBandwidth(double b) {
	using Kaiser = signalsmith::windows::Kaiser;
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

TEST("Window comparison (Kaiser)") {
	using Kaiser = signalsmith::windows::Kaiser;
//	using Confined = signalsmith::windows::ApproximateConfinedGaussian;

	Plot2D bandwidthPeakPlot, bandwidthEnergyPlot, bandwidthEnbwPlot;
	auto _1 = bandwidthPeakPlot.writeLater("windows-kaiser-sidelobe-peaks.svg");
	auto _2 = bandwidthEnergyPlot.writeLater("windows-kaiser-sidelobe-energy.svg");
	auto _3 = bandwidthEnbwPlot.writeLater("windows-kaiser-enbw.svg");
	
	bandwidthPeakPlot.x.linear(1, 10).label("bandwidth");
	bandwidthPeakPlot.y.linear(-120, 0).label("side/main peaks (dB)");
	for (int i = 1; i <= 10; ++i) bandwidthPeakPlot.x.minor(i);
	for (int i = -120; i <= 0; i += 20) bandwidthPeakPlot.y.minor(i);
	bandwidthEnergyPlot.x.copyFrom(bandwidthPeakPlot.x);
	bandwidthEnergyPlot.y.copyFrom(bandwidthPeakPlot.y).label("side/main energy (dB)");
	bandwidthEnbwPlot.x.linear(0.1, 22).major(0.1, "").minors(5, 10, 15, 20).label("bandwidth");
	bandwidthEnbwPlot.y.label("ENBW").linear(0, 3.5).major(0).minors(1, 2, 3);
	
	auto &linePeakPlain = bandwidthPeakPlot.line();
	auto &linePeakHeuristic = bandwidthPeakPlot.line();
	auto &lineEnergyPlain = bandwidthEnergyPlot.line();
	auto &lineEnergyHeuristic = bandwidthEnergyPlot.line();
	auto &lineEnbwPlain = bandwidthEnbwPlot.line();
	auto &lineEnbwHeuristic = bandwidthEnbwPlot.line();
	bandwidthPeakPlot.legend(1, 1).add(linePeakPlain, "exact").add(linePeakHeuristic, "heuristic");
	bandwidthEnergyPlot.legend(1, 1).add(lineEnergyPlain, "exact").add(lineEnergyHeuristic, "heuristic");
	bandwidthEnbwPlot.legend(1, 0).add(lineEnbwPlain, "exact").add(lineEnbwHeuristic, "heuristic");

	for (double b = 0.1; b < 22; b += 0.1) {
		auto kaiserPlain = signalsmith::windows::Kaiser::withBandwidth(b, false);
		auto kaiserHeuristic = signalsmith::windows::Kaiser::withBandwidth(b, true);
		auto statsPlain = measureWindow(kaiserPlain, b);
		auto statsHeuristic = measureWindow(kaiserHeuristic, b);

		{
			double energyDb = energyToDb(statsPlain.sideEnergy/(statsPlain.mainEnergy + 1e-100));
			lineEnergyPlain.add(b, energyDb);
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

			double peakDb = ampToDb(statsPlain.sidePeak/(statsPlain.mainPeak + 1e-100));
			linePeakPlain.add(b, peakDb);
			{ // Approximate peak ratio
				double predictedPeakDb = Kaiser::bandwidthToPeakDb(b);
				if (b >= 2 && b <= 10 && std::abs(peakDb - predictedPeakDb) > 0.5) {
					test.log(b, ": ", peakDb, " ~= ", predictedPeakDb);
					return test.fail("Peak approximation should be accurate within 0.5dB, within 2-10x range");
				}
				double predictedBandwidth = Kaiser::peakDbToBandwidth(predictedPeakDb);
				if (std::abs(b - predictedBandwidth) > 1e-3) {
					test.log(b, " != ", predictedBandwidth, " @ ", predictedPeakDb);
					return test.fail("inverse of predicted bandwidth (peak)");
				}
			}
		}

		{
			double energyDb = energyToDb(statsHeuristic.sideEnergy/(statsHeuristic.mainEnergy + 1e-100));
			lineEnergyHeuristic.add(b, energyDb);
			{ // Approximate energy ratio
				double predictedEnergyDb = Kaiser::bandwidthToEnergyDb(b, true);
				if (b >= 0.5 && b <= 10 && std::abs(energyDb - predictedEnergyDb) > 0.5) {
					test.log(b, ": ", energyDb, " ~= ", predictedEnergyDb);
					return test.fail("Energy approximation (heuristic) should be accurate within 0.5dB, within 2-10x range");
				}
				double predictedBandwidth = Kaiser::energyDbToBandwidth(predictedEnergyDb, true);
				if (std::abs(b - predictedBandwidth) > 1e-3) {
					test.log(b, " != ", predictedBandwidth, " @ ", predictedEnergyDb);
					return test.fail("inverse of predicted bandwidth (heuristic energy)");
				}
			}

			double peakDb = ampToDb(statsHeuristic.sidePeak/(statsHeuristic.mainPeak + 1e-100));
			linePeakHeuristic.add(b, peakDb);
			{ // Approximate peak ratio
				double predictedPeakDb = Kaiser::bandwidthToPeakDb(b, true);
				if (b >= 0.5 && b <= 10 && std::abs(peakDb - predictedPeakDb) > 0.5) {
					test.log(b, ": ", peakDb, " ~= ", predictedPeakDb);
					return test.fail("Peak approximation (heuristic) should be accurate within 0.5dB, within 2-10x range");
				}
				double predictedBandwidth = Kaiser::peakDbToBandwidth(predictedPeakDb, true);
				if (b > 0.5 && std::abs(b - predictedBandwidth) > 1e-3) {
					test.log(b, " != ", predictedBandwidth, " @ ", predictedPeakDb);
					return test.fail("inverse of predicted bandwidth (heuristic peak)");
				}
			}
		}

		lineEnbwPlain.add(b, statsPlain.enbw);
		lineEnbwHeuristic.add(b, statsHeuristic.enbw);
		
		if (std::abs(statsPlain.enbw - Kaiser::bandwidthToEnbw(b)) > 0.05) {
			test.log(b, ": ", statsPlain.enbw, " != ", Kaiser::bandwidthToEnbw(b));
			return test.fail("Predicted ENBW");
		}
		if (std::abs(statsHeuristic.enbw - Kaiser::bandwidthToEnbw(b, true)) > 0.05) {
			test.log(b, ": ", statsHeuristic.enbw, " != ", Kaiser::bandwidthToEnbw(b, true));
			return test.fail("Predicted heuristic ENBW");
		}
	}
}
