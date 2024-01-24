#include <test/tests.h>
#include "../common.h"

#include "curves.h"

#include <cmath>

TEST("Reciprocal Bark-scale approximation") {
	struct Pair {
		double centre;
		double bandwidth;
	};
	std::vector<Pair> observed{
		{50, 80},
		{150, 100},
		{250, 100},
		{350, 100},
		{450, 110},
		{570, 120},
		{700, 140},
		{840, 150},
		{1000, 160},
		{1170, 190},
		{1370, 210},
		{1600, 240},
		{1850, 280},
		{2150, 320},
		{2500, 380},
		{2900, 450},
		{3400, 550},
		{4000, 700},
		{4800, 900},
		{5800, 1100},
		{7000, 1300},
		{8500, 1800},
		{10500, 2500},
		{13500, 3500}
	};
	
	auto barkScale = signalsmith::curves::Reciprocal<double>::barkScale();

	signalsmith::plot::Figure figure;
	auto &plot = figure(0, 0).plot(350, 200);

	auto &approxFreq = plot.line(0);
	approxFreq.styleIndex.marker = 3; // hollow circle
	auto &approxBw = plot.line(3);
	approxBw.styleIndex.dash = 0;
	approxBw.styleIndex.marker = 1; // hollow diamond
	for (double bark = 0.25; bark <= 27.5; bark += 0.01) {
		double hz = barkScale(bark);
		approxFreq.add(bark, hz);
		approxBw.add(bark, barkScale.dx(bark));
	}

	for (int b = 1; b <= 24; ++b) {
		auto &pair = observed[b - 1];
		approxFreq.marker(b, pair.centre);
		approxBw.marker(b, pair.bandwidth);
	}

	plot.y.range(std::log, 30, 20000).major(100, "100 Hz").major(1000, "1 kHz").major(10000, "10 kHz");

	plot.x.linear(0.39, 25.5).label("Bark scale").majors(1, 24);
	for (int i = 2; i <= 9; ++i) {
		if (i >= 3) plot.y.minor(10*i, "");
		plot.y.minor(100*i, "");
		plot.y.minor(1000*i, "");
	}
	plot.y.minor(20000, "");
	for (int i = 2; i <= 23; ++i) {
		plot.x.tick(i, " ");
	}
	
	approxFreq.label(16, 4000, "frequency", 180, 0);
	approxBw.label(16, 300, "bandwidth", 0, 0);

	figure.write("curves-reciprocal-approx-bark.svg");
	test.pass();
}
