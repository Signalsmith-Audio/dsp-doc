#include "../common.h"

#include <complex>
#include <cmath>
#include <test/tests.h>

#include "./window-stats.h"

TEST("Window examples (ACG)") {
	Figure figure;
	auto &timePlot = figure(0, 0).plot(350, 150);
	timePlot.x.linear(0, 1).ticks(0, 1);
	timePlot.y.linear(0, 1).major(0).minor(1);
	auto &legend = timePlot.legend(1.5, 1);
	
	int size = 2048;
	auto addSigma = [&](double sigma, std::string label) {
		auto &line = timePlot.line();
		signalsmith::windows::ApproximateConfinedGaussian acg(sigma);
		std::vector<double> window(size);
		acg.fill(window, size);
		for (int i = 0; i < size; ++i) {
			line.add((i + 0.5)/size, window[i]);
		}
		legend.add(line, label);
	};
	addSigma(0.5, "σ=1/2");
	addSigma(0.25, "σ=1/4");
	addSigma(0.125, "σ=1/8");
	addSigma(0.0625, "σ=1/16");

	figure.write("windows-acg-examples.svg");
	return test.pass();
}

/*
// Should be close to 1dB of optimal sigma value, according to our weird homebrewed metric below
double optimalAcgHeuristic(double bandwidth) {
	return 0.3/std::sqrt(bandwidth);// + 0.5/(bandwidth*bandwidth*bandwidth*bandwidth);
}
TEST("Optimal ACG search") {
	using Confined = signalsmith::windows::ApproximateConfinedGaussian;
	auto score = [&](double sigma, double bandwidth) {
		auto confined = Confined(sigma);
		auto stats = measureWindow(confined, bandwidth);
		double energyRatio = stats.sideEnergy/(stats.mainEnergy + 1e-100);
		double peakRatio = stats.sidePeak/(stats.mainPeak + 1e-100);
		return energyRatio + peakRatio*0.1/bandwidth; // weird homebrewed metric
	};
	auto findGoodSigma = [&](double bandwidth) {
		double bestSigma = 0;
		double bestScore = 1e100;
		double step = 1e-3;
		bool foundNewBest = true;
		for (double s = step; s < 0.5 || (s < 10 && foundNewBest); s = std::max(s + step, s*1.01)) {
			double candidateScore = score(s, bandwidth);
			bool foundNewBest = candidateScore < bestScore;
			if (foundNewBest) {
				bestSigma = s;
				bestScore = candidateScore;
			}
		}
		return bestSigma;
	};
	
	Plot2D plot(500, 250);
	plot.y.linear(0, 2).major(0).minors(1, 2);
	plot.x.linear(0, 1).minors(0, 1).ticks(0.3, 0.35, 0.4);
	auto &line = plot.line();
	auto &line2 = plot.line();
	for (double b = 1; b <= 1000; b = std::max(b + 0.01, b*1.05)) {
		double sigma = findGoodSigma(b);
		double heuristic = Confined::bandwidthToSigma(b);
		test.log("bandwidth: ", b, " -> ", sigma, "\t", score(sigma, b)/score(heuristic, b));
		line.add(1/b, sigma);
		line2.add(1/b, heuristic);
		plot.write("tmp-window-acg-good.svg");
	}
	
	plot.write("tmp-window-acg-good.svg");
}
*/

TEST("Window comparison (ACG)") {
	using Confined = signalsmith::windows::ApproximateConfinedGaussian;

	Plot2D bandwidthPeakPlot, bandwidthEnergyPlot, bandwidthEnbwPlot;
	auto _1 = bandwidthPeakPlot.writeLater("windows-acg-sidelobe-peaks.svg");
	auto _2 = bandwidthEnergyPlot.writeLater("windows-acg-sidelobe-energy.svg");
	auto _3 = bandwidthEnbwPlot.writeLater("windows-acg-enbw.svg");
	
	bandwidthPeakPlot.x.linear(1, 10).label("bandwidth");
	bandwidthPeakPlot.y.linear(-80, 0).label("side/main peaks (dB)");
	for (int i = 1; i <= 10; ++i) bandwidthPeakPlot.x.minor(i);
	for (int i = -80; i <= 0; i += 20) bandwidthPeakPlot.y.minor(i);
	bandwidthEnergyPlot.x.copyFrom(bandwidthPeakPlot.x);
	bandwidthEnergyPlot.y.copyFrom(bandwidthPeakPlot.y).label("side/main energy (dB)");
	bandwidthEnbwPlot.x.linear(0.1, 22).major(0.1, "").minors(5, 10, 15, 20).label("bandwidth");
	bandwidthEnbwPlot.y.label("ENBW").linear(0, 3.5).major(0).minors(1, 2, 3);
	
	auto &linePeakPlain = bandwidthPeakPlot.line();
	auto &lineEnergyPlain = bandwidthEnergyPlot.line();
	auto &lineEnbwPlain = bandwidthEnbwPlot.line();
	auto &linePeakForced = bandwidthPeakPlot.line();
	auto &lineEnergyForced = bandwidthEnergyPlot.line();
	auto &lineEnbwForced = bandwidthEnbwPlot.line();
	bandwidthPeakPlot.legend(1, 1).add(linePeakPlain, "natural").add(linePeakForced, "forced P-R");
	bandwidthEnergyPlot.legend(1, 1).add(lineEnergyPlain, "natural").add(lineEnergyForced, "forced P-R");
	bandwidthEnbwPlot.legend(1, 0).add(lineEnbwPlain, "natural").add(lineEnbwForced, "forced P-R");

	for (double b = 0.1; b < 22; b += 0.1) {
		auto confined = Confined::withBandwidth(b);
		auto stats = measureWindow(confined, b);
		auto statsForced = measureWindow(confined, b, true);

		{
			double energyDb = energyToDb(stats.sideEnergy/(stats.mainEnergy + 1e-100));
			lineEnergyPlain.add(b, energyDb);

			double peakDb = ampToDb(stats.sidePeak/(stats.mainPeak + 1e-100));
			linePeakPlain.add(b, peakDb);
		}
		{
			double energyDb = energyToDb(statsForced.sideEnergy/(statsForced.mainEnergy + 1e-100));
			lineEnergyForced.add(b, energyDb);

			double peakDb = ampToDb(statsForced.sidePeak/(statsForced.mainPeak + 1e-100));
			linePeakForced.add(b, peakDb);
		}

		lineEnbwPlain.add(b, stats.enbw);
		lineEnbwForced.add(b, statsForced.enbw);
	}
	return test.pass();
}
