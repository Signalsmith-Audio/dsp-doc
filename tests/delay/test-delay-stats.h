#include "fft.h"

#include "../common.h"

#include <string>
#include <vector>
#include <complex>
#include <cmath>
#include <iostream>

template<typename Sample, bool useReadWrite, class Test, class Delay>
void simpleImpulseComparison(Test &test, Delay &delay) {
	int impulseTime = 10;
	int delaySamples = std::max(100, int(Delay::latency) + 10);
	
	double compensatedDelay = delaySamples - Delay::latency;
	if (compensatedDelay < (useReadWrite ? 0 : 1)) return test.fail("Delay latency too high to test");
	
	int maxLength = delaySamples, testLength = maxLength + impulseTime + delaySamples;
	delay.resize(maxLength);
	
	for (int i = 0; i < testLength; ++i) {
		Sample input = (i == impulseTime ? 1 : 0);
		Sample expected = (i == impulseTime + delaySamples) ? 1 : 0;

		Sample output;
		if (useReadWrite) {
			output = delay.readWrite(input, compensatedDelay);
		} else {
			output = delay.read(compensatedDelay);
			delay.write(input);
		}
		if (!test.closeEnough(expected, output, "impulse matches")) {
			std::cout << "impulse at " << i << ":\t" << output << " != " << expected << "\n";
		}
	}
}

struct ImpulseAnalyser {
	using Sample = double;
	using Complex = std::complex<Sample>;
	
	int impulseLength;
	signalsmith::fft::RealFFT<Sample> realFft;
	
	ImpulseAnalyser(int impulseLength=512) : impulseLength(impulseLength), realFft(impulseLength) {}

	template<class Delay>
	void getFractionalImpulse(Delay &delay, Sample delaySamples, std::vector<Sample> &result) {
		delay.resize(1);
		result.resize(impulseLength);
		
		for (int i = 0; i < impulseLength; ++i) {
			Sample value = (i == 0) ? 1 : 0;
			result[i] = delay.readWrite(value, delaySamples);
		}
	}
	
	struct Result {
		int bands, hBands;
		std::vector<Sample> aliasing;
		std::vector<Sample> ampMean;
		std::vector<Sample> ampMax;
		std::vector<Sample> ampMin;
		std::vector<Sample> delayMax;
		std::vector<Sample> delayMin;
		
		Result(int bands) : bands(bands), hBands(bands/2 + 1), aliasing(hBands), ampMean(hBands), ampMax(hBands), ampMin(hBands), delayMax(hBands), delayMin(hBands) {}
		
		struct BandwidthResult {
			Sample aliasing = -1e10;
			Sample dbMax = -1e10;
			Sample dbMin = 1e10;
			Sample delayMax = -1e10;
			Sample delayMin = 1e10;
			
			void print(std::ostream &out) {
				out << "\taliasing = " << aliasing << " dB\n";
				out << "\tamp = " << dbMin << " to " << dbMax << " dB\n";
				out << "\tdelay error = " << delayMin << " to " << delayMax << " samples\n";
			}
		};
		BandwidthResult performanceForBandwidth(double ratio=0.45) {
			BandwidthResult result;
			for (int i = 1; i < ratio*bands + 0.5 && i < bands/2; ++i) {
				result.aliasing = std::max(result.aliasing, energyToDb(aliasing[i]));
				result.dbMax = std::max(result.dbMax, ampToDb(ampMax[i]));
				result.dbMin = std::min(result.dbMin, ampToDb(ampMin[i]));
				result.delayMax = std::max(result.delayMax, delayMax[i]);
				result.delayMin = std::min(result.delayMin, delayMin[i]);
			}
			return result;
		}

		template<class Test, class Limits>
		void test(Test &t, const Limits &bandwidth, const Limits &aliasing) {
			Limits ampLow, ampHigh, delayError;
			int count = sizeof(bandwidth)/sizeof(double);
			double inf = HUGE_VAL;
			for (int i = 0; i < count; ++i) {
				ampLow[i] = -inf;
				ampHigh[i] = inf;
				delayError[i] = inf;
			}
			
			test(t, bandwidth, aliasing, ampLow, ampHigh, delayError);
		}
			
		template<class Limits>
		void printValid(std::ostream &out, const Limits &bandwidth) {
			int count = sizeof(bandwidth)/sizeof(double);
			
			auto roundUp = [&](double value) {
				if (std::abs(value) < 1e-5) {
					return 1e-4;
				} else if (std::abs(value) > 1.2) {
					return std::ceil(value*2)*0.5;
				} else if (std::abs(value) >= 0.5) {
					return std::ceil(value*10)*0.1;
				} else if (std::abs(value) >= 0.12) {
					return std::ceil(value*20)*0.05;
				} else {
					return std::ceil(value*100)*0.01;
				}
			};
			out << "Valid ranges:\n";
			out << "\tdouble bandwidth[] = {";
			for (int i = 0; i < count; ++i) {
				auto b = bandwidth[i];
				out << (i == 0 ? "" : ", ") << b;
			}
			out << "};\n";
			out << "\tdouble aliasing[] = {";
			for (int i = 0; i < count; ++i) {
				auto b = bandwidth[i];
				auto p = performanceForBandwidth(b*0.005);
				out << (i == 0 ? "" : ", ") << roundUp(p.aliasing + 0.1);
			}
			out << "};\n";
			out << "\tdouble ampLow[] = {";
			for (int i = 0; i < count; ++i) {
				auto b = bandwidth[i];
				auto p = performanceForBandwidth(b*0.005);
				out << (i == 0 ? "" : ", ") << -roundUp(-p.dbMin*1.01);
			}
			out << "};\n";
			out << "\tdouble ampHigh[] = {";
			for (int i = 0; i < count; ++i) {
				auto b = bandwidth[i];
				auto p = performanceForBandwidth(b*0.005);
				out << (i == 0 ? "" : ", ") << roundUp(p.dbMax*1.01);
			}
			out << "};\n";
			out << "\tdouble delayError[] = {";
			for (int i = 0; i < count; ++i) {
				auto b = bandwidth[i];
				auto p = performanceForBandwidth(b*0.005);
				double maxDelayError = std::max(std::abs(p.delayMin), std::abs(p.delayMax));
				out << (i == 0 ? "" : ", ") << roundUp(maxDelayError*1.01);
			}
			out << "};\n";
		}
		
		template<class Test, class Limits>
		void test(Test &test, const Limits &bandwidth, const Limits &aliasing, const Limits &ampLow, const Limits &ampHigh, const Limits &delayError) {
			int count = sizeof(bandwidth)/sizeof(double);
			double toleranceDb = 0.001;
			double toleranceSamples = 0.0001;
			for (int i = 0; i < count; ++i) {
				std::string bandwidthString = std::to_string(bandwidth[i]) + "% bandwidth";
				auto performance = performanceForBandwidth(0.005*bandwidth[i]);
				if (performance.aliasing > aliasing[i] + toleranceDb) {
					performance.print(std::cout);
					printValid(std::cout, bandwidth);
					return test.fail("Aliasing too high at " + bandwidthString);
				}
				if (performance.dbMin < ampLow[i] - toleranceDb) {
					performance.print(std::cout);
					printValid(std::cout, bandwidth);
					return test.fail("Amp too low at " + bandwidthString);
				}
				if (performance.dbMax > ampHigh[i] + toleranceDb) {
					performance.print(std::cout);
					printValid(std::cout, bandwidth);
					return test.fail("Amp too high at " + bandwidthString);
				}
				double maxDelayError = std::max(std::abs(performance.delayMin), std::abs(performance.delayMax));
				if (maxDelayError > delayError[i] + toleranceSamples) {
					performance.print(std::cout);
					printValid(std::cout, bandwidth);
					return test.fail("Delay error at " + bandwidthString);
				}
			}
		}
	};
	
	static double ampToDb(double amp) {
		return 20*log10(std::abs(amp) + 1e-30);
	}
	static double energyToDb(double energy) {
		return 10*log10(std::abs(energy) + 1e-30);
	}
	
	template<class Delay>
	Result analyseFractional(Delay &delay, std::string name, int steps, Sample endDelay=1) {
		Result result(impulseLength);

		std::vector<Sample> delays(steps);
		std::vector<std::vector<Sample>> impulses(steps);
		std::vector<std::vector<Complex>> spectra(steps);
		std::vector<std::vector<Sample>> delayErrors(steps);
		auto &aliasing = result.aliasing;
		
		Sample singleSamplePhaseStep = -2*M_PI/impulseLength;
		
		// Calculate spectra
		for (int d = 0; d < steps; ++d) {
			Sample delaySamples = d*endDelay/(steps - 1);
			Sample phaseStep = singleSamplePhaseStep*(delaySamples + delay.latency);

			delays[d] = delaySamples;
			getFractionalImpulse(delay, delaySamples, impulses[d]);

			spectra[d].resize(impulseLength/2);
			realFft.fft(impulses[d], spectra[d]);
			for (int i = 1; i < impulseLength/2; ++i) {
				Sample phase = phaseStep*i;
				spectra[d][i] *= Complex{cos(phase), -sin(phase)};
			}
		}
		auto getBin = [&](int d, int i) {
			if (i <= 0) return Complex(spectra[d][0].real());
			if (i >= impulseLength/2) return Complex(spectra[d][0].imag());
			return spectra[d][i];
		};
		
		// Calculate aliasing and various min/max
		for (int i = 0; i <= impulseLength/2; ++i) {
			Complex sum = 0;
			Sample sum2 = 0;
			Sample energyMax = std::norm(getBin(0, i));
			Sample energyMin = energyMax;
			for (int d = 0; d < steps; ++d) {
				Complex value = getBin(d, i);
				Sample energy = std::norm(value);
				if (energy > energyMax) energyMax = energy;
				if (energy < energyMin) energyMin = energy;
				sum += value;
				sum2 += energy;
			}
			sum /= steps;
			sum2 /= steps;
			Sample variance = sum2 - std::norm(sum);
			aliasing[i] = variance;
			result.ampMean[i] = std::sqrt(sum2);
			
			result.ampMax[i] = std::sqrt(energyMax);
			result.ampMin[i] = std::sqrt(energyMin);
		}
		// Calculate delay errors
		for (int d = 0; d < steps; ++d) {
			delayErrors[d].resize(impulseLength/2 + 1);
			for (int i = 0; i <= impulseLength/2; ++i) {
				Complex value = getBin(d, i);
				Complex prev = getBin(d, i - 1);
				Complex delayComplex = value*std::conj(prev);
				double delayAngle = std::arg(delayComplex);
				double delayError = delayAngle/singleSamplePhaseStep;
				delayErrors[d][i] = delayError;
			}
		}
		for (int i = 0; i <= impulseLength/2; ++i) {
			Sample delayMin = delayErrors[0][i], delayMax = delayMin;
			for (int d = 0; d < steps; ++d) {
				Sample delay = delayErrors[d][i];
				if (delay > delayMax) delayMax = delay;
				if (delay < delayMin) delayMin = delay;
			}
			result.delayMax[i] = delayMax;
			result.delayMin[i] = delayMin;
		}
		
		/*/// CSV output ///*/

		// Output aliasing
		CsvWriter aliasWriter(name + ".fractional-stats");
		aliasWriter.line("freq", "aliasing", "mean-dB", "min-dB", "max-dB", "min-delay", "max-delay");
		for (int i = 0; i <= impulseLength/2; ++i) {
			Sample f = (Sample)i/impulseLength;
			aliasWriter.line(f, energyToDb(aliasing[i]), ampToDb(result.ampMean[i]), ampToDb(result.ampMin[i]), ampToDb(result.ampMax[i]), result.delayMin[i], result.delayMax[i]);
		}
		// Output impulses
		CsvWriter impulseWriter(name + ".fractional-impulse");
		impulseWriter.write("sample");
		for (int d = 0; d < steps; ++d) {
			impulseWriter.write(delays[d]);
		}
		impulseWriter.line();
		for (int i = 0; i < impulseLength; ++i) {
			impulseWriter.write(i);
			for (int d = 0; d < steps; ++d) {
				impulseWriter.write(impulses[d][i]);
			}
			impulseWriter.line();
		}

		// Write amp/phase
		CsvWriter ampWriter(name + ".fractional-amplitude");
		CsvWriter phaseWriter(name + ".fractional-phase");
		CsvWriter groupWriter(name + ".fractional-group");
		// Header
		ampWriter.write("freq");
		phaseWriter.write("freq");
		groupWriter.write("freq");
		for (int d = 0; d < steps; ++d) {
			ampWriter.write(delays[d]);
			phaseWriter.write(delays[d]);
			groupWriter.write(delays[d]);
		}
		ampWriter.line();
		phaseWriter.line();
		groupWriter.line();
		// Body
		for (int i = 0; i <= impulseLength/2; ++i) {
			Sample f = (Sample)i/impulseLength;
			ampWriter.write(f);
			phaseWriter.write(f);
			groupWriter.write(f);
			for (int d = 0; d < steps; ++d) {
				Complex value = getBin(d, i);
				double norm = std::norm(value);
				ampWriter.write(energyToDb(norm));
				double angle = std::arg(value);
				phaseWriter.write(angle);
				groupWriter.write(delayErrors[d][i]);
			}
			ampWriter.line();
			phaseWriter.line();
			groupWriter.line();
		}
		return result;
	}
};

template<typename Sample, class Test, class Delay>
ImpulseAnalyser::Result collectFractionalDelayStats(Test &test, Delay &delay, std::string name, bool impulseComparison=true) {
	if (impulseComparison) {
		simpleImpulseComparison<Sample, false>(test, delay);
		simpleImpulseComparison<Sample, true>(test, delay);
	}
	
	ImpulseAnalyser analyser;
	return analyser.analyseFractional(delay, name, 31, 1 - 1e-7);
};
