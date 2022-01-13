#include <test/tests.h>
#include "../common.h"

#include "envelopes.h"

#include "fft.h"

#include <array>
#include <cmath>
#include <complex>

struct Result {
	std::vector<std::vector<double>> impulses;
	std::vector<std::vector<std::complex<double>>> spectra;
};
void analyseStack(Result &result, Test &test, int length, const std::vector<int> &depths) {
	std::vector<signalsmith::envelopes::BoxStackFilter<double>> filters;
	for (auto depth : depths) {
		filters.emplace_back(length, depth);
	}

	std::vector<std::vector<double>> impulses;
	std::vector<std::vector<std::complex<double>>> spectra;
	
	impulses.resize(depths.size());
	spectra.resize(depths.size());

	for (int i = 0; i < length; ++i) {
		double v = (i == 0) ? 1 : 0;
		for (size_t di = 0; di < depths.size(); ++di) {
			double r = filters[di](v);
			impulses[di].push_back(r);
			TEST_ASSERT(r != 0); // non-zero impulse for whole range
		}
	}
	for (auto &filter : filters) {
		double v = filter(0);
		TEST_ASSERT(std::abs(v) < 1e-16); // should get back to 0
	}
	
	int fftSize = 32768;
	// Zero-pad
	for (auto &impulse : impulses) impulse.resize(fftSize, 0);
	
	signalsmith::fft::FFT<double> fft(fftSize);
	for (size_t di = 0; di < depths.size(); ++di) {
		spectra[di].resize(fftSize, 0);
		fft.fft(impulses[di], spectra[di]);
	}
	
	result.impulses = impulses;
	result.spectra = spectra;
}

TEST("Box stack", box_stack) {
	std::vector<int> depths = {2, 4, 6};
	Result longResult, shortResult;
	analyseStack(longResult, test, 1000, depths);
	analyseStack(shortResult, test, 30, depths);

	{
		CsvWriter csv("box-stack-long-time");
		csv.write("i");
		for (auto depth : depths) csv.write(depth);
		csv.line();
		for (size_t i = 0; i < 1000; ++i) {
			csv.write(i);
			for (auto &impulse : longResult.impulses) {
				csv.write(impulse[i]);
			}
			csv.line();
		}
	}
	{
		CsvWriter csv("box-stack-long-freq");
		csv.write("f");
		for (auto depth : depths) csv.write(depth);
		csv.line();
		size_t fftSize = longResult.spectra[0].size();
		size_t length = 1000;
		for (size_t f = 0; f < fftSize/2; ++f) {
			double relativeF = f*1.0*length/fftSize;
			csv.write(relativeF);
			for (size_t di = 0; di < depths.size(); ++di) {
				auto &spectrum = longResult.spectra[di];
				double norm = std::norm(spectrum[f]);
				double db = 10*std::log10(norm + 1e-100);
				csv.write(db);
			}
			csv.line();
		}
	}
	{
		CsvWriter csv("box-stack-short-freq");
		csv.write("f");
		for (auto depth : depths) csv.write(depth);
		csv.line();
		size_t fftSize = shortResult.spectra[0].size();
		size_t length = 30;
		for (size_t f = 0; f < fftSize/2; ++f) {
			csv.write(f*1.0*length/fftSize);
			for (auto &spectrum : shortResult.spectra) {
				double norm = std::norm(spectrum[f]);
				double db = 10*std::log10(norm + 1e-100);
				csv.write(db);
			}
			csv.line();
		}
	}

	depths.clear();
	for (int i = 1; i < 12; ++i) {
		depths.push_back(i);
	}
	analyseStack(longResult, test, 1000, depths);
	{
		size_t fftSize = longResult.spectra[0].size();
		size_t length = 1000;
		for (size_t f = 0; f < fftSize/2; ++f) {
			double relativeF = f*1.0*length/fftSize;
			for (size_t di = 0; di < depths.size(); ++di) {
				auto &spectrum = longResult.spectra[di];
				double norm = std::norm(spectrum[f]);
				double db = 10*std::log10(norm + 1e-100);
				
				double bandwidth = signalsmith::envelopes::BoxStackFilter<double>::layersToBandwidth(depths[di]);
				double stopDb = signalsmith::envelopes::BoxStackFilter<double>::layersToPeakDb(depths[di]);
				if (relativeF >= bandwidth*0.5) {
					TEST_ASSERT(db <= stopDb*0.98); // at most 2% off in dB terms
				}
			}
		}
	}
}

TEST("Box stack properties", box_stack_properties) {
	using Filter = signalsmith::envelopes::BoxStackFilter<float>;
	CsvWriter csv("box-stack-stats");
	csv.line("n", "bandwidth", "peak (dB)");
	for (int n = 1; n < 20; ++n) {
		csv.line(n, Filter::layersToBandwidth(n), Filter::layersToPeakDb(n));
	}
	return test.pass();
}
