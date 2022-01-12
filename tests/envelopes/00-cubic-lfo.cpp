#include <test/tests.h>
#include "../common.h"

#include "envelopes.h"
#include "spectral.h"
#include "windows.h"

#include <array>

TEST("Cubic LFO", cubic_lfo) {
	long seed = 12345;
	signalsmith::envelopes::CubicLfo lfoA(seed), lfoB(seed), lfoC(seed), lfoD(seed), lfoE(seed);
	
	lfoA.set(8, 10, 0.02, 0, 0);
	lfoB.set(6, 8, 0.02, 1, 0);
	lfoC.set(4, 6, 0.02, 0, 0.5);
	lfoD.set(2, 4, 0.02, 0, 1);
	lfoE.set(0, 2, 0.02, 1, 1);
	
	CsvWriter csv("cubic-lfo-example");
	csv.line('i', "no random", "freq", "depth (50%)", "depth (100%)", "both");
	
	for (int i = 0; i < 500; ++i) {
		csv.line(i, lfoA.next(), lfoB.next(), lfoC.next(), lfoD.next(), lfoE.next());
	}
	return test.pass();
}

TEST("Cubic LFO (spectrum)", cubic_lfo_spectrum) {
	constexpr int count = 5;
	std::array<double, count> factors = {0, 0.2, 0.4, 0.6, 0.8};

	double rate = 1.0/20.382;
	int duration = 8192;
	int trials = 1000;
	std::vector<double> waveform(duration);
	std::vector<std::complex<double>> spectrum(duration);

	auto writeSpectra = [&](std::string name, double freqRandom, double ampRandom) {
		std::vector<std::vector<double>> spectralEnergy;
		using Kaiser = signalsmith::windows::Kaiser;
		auto kaiser = Kaiser::withBandwidth(Kaiser::energyDbToBandwidth(-120));
		signalsmith::spectral::WindowedFFT<double> fft(duration, [&](double r) {
			return kaiser(r);
		});
		
		for (auto f : factors) {
			std::vector<double> energy(duration);
			for (int r = 0; r < trials; ++r) {
				signalsmith::envelopes::CubicLfo lfo;
				lfo.set(-1, 1, rate, f*freqRandom, f*ampRandom);

				for (int i = 0; i < duration; ++i) {
					waveform[i] = lfo.next();
				}
				fft.fft(waveform, spectrum);
				for (int f = 0; f < duration; ++f) {
					energy[f] += std::norm(spectrum[f]);
				}
			}
			spectralEnergy.push_back(std::move(energy));
		}
		
		CsvWriter csv(name);
		csv.write("f");
		for (auto f : factors) {
			csv.write(f);
		}
		csv.line();
		for (int f = 0; f < duration; ++f) {
			csv.write(f*1.0/duration/rate);
			for (int i = 0; i < count; ++i) {
				csv.write(spectralEnergy[i][f]);
			}
			csv.line();
		}
	};
	writeSpectra("cubic-lfo-spectrum-freq", 1, 0);
	writeSpectra("cubic-lfo-spectrum-depth", 0, 1);
	
	return test.pass();
}

TEST("Cubic LFO (changes)", cubic_lfo_changes) {
	double low = 0, high = 1;
	double rate = 0.01;
	signalsmith::envelopes::CubicLfo lfo, randomLfo(12345);
	
	CsvWriter csv("cubic-lfo-changes");
	csv.line("i", "LFO", "randomised", "low", "high");
	int i = 0;
	auto writeSamples = [&](int length) {
		lfo.set(low, high, rate);
		randomLfo.set(low, high, rate, 1, 0.4);
		int end = i + length;
		for (; i < end; ++i) {
			csv.line(i, lfo.next(), randomLfo.next(), low, high);
		}
	};
	writeSamples(430);
	high = 2;
	low = 0.5;
	writeSamples(285);
	low = -2;
	high = - 1;
	writeSamples(230);

	return test.pass();
}
