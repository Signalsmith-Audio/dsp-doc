#include <vector>
#include <complex>

using Spectrum = std::vector<std::complex<double>>;

double ampToDb(double amp) {
	return 20*std::log10(std::max(amp, 1e-100));
}

template<class Filter>
Spectrum getSpectrum(Filter &filter, double impulseLimit=1e-10) {
	filter.reset();
	Spectrum impulse, spectrum;

	// Size depends on impulse length, to guarantee sufficient frequency resolution
	int sample = 0;
	int belowLimitCounter = 0;
	while (sample < 100 || belowLimitCounter <= sample*0.9 /*overshoot by 10x for padding */) {
		double v = filter((sample == 0) ? 1 : 0);
		impulse.push_back(v);
		auto mag = std::abs(v);
		if (mag < impulseLimit) {
			++belowLimitCounter;
		} else {
			belowLimitCounter = 0;
		}
		++sample;
	}
	
	int pow2 = 1;
	while (pow2 < sample) pow2 *= 2;
	while (sample < pow2) {
		impulse.push_back(filter(0));
		++sample;
	}
	
	signalsmith::FFT<double> fft(pow2);
	spectrum.resize(pow2);
	fft.fft(impulse, spectrum);
	return spectrum;
}

double interpSpectrum(Spectrum spectrum, double freq) {
	double index = freq*spectrum.size();
	int intFreqLow = index;
	double ratio = index - intFreqLow;
	int intFreqHigh = intFreqLow + 1;
	if (intFreqHigh >= (int)spectrum.size()) {
		--intFreqLow;
		--intFreqHigh;
	}
	double energyLow = std::norm(spectrum[intFreqLow]);
	double energyHigh = std::norm(spectrum[intFreqHigh]);
	return std::sqrt(energyLow + (energyHigh - energyLow)*ratio);
}

void writeSpectrum(Spectrum spectrum, std::string name) {
	CsvWriter csv(name);
	csv.line("freq", "dB", "phase", "group delay");

	double prevPhase = 0, prevMag = 0;
	for (size_t i = 0; i <= spectrum.size()/2; ++i) {
		auto bin = spectrum[i];
		double mag = std::abs(bin);
		double db = 20*std::log10(mag);
		double phase = std::arg(bin);
		double phaseDiff = (prevPhase - phase);
		if (mag < 1e-10 || prevMag < 1e-10) {
			phaseDiff = 0;
		} else if (phaseDiff > M_PI) {
			phaseDiff -= 2*M_PI;
		} else if (phaseDiff <= -M_PI) {
			phaseDiff += 2*M_PI;
		}
		double groupDelay = phaseDiff*spectrum.size()/(2*M_PI);
		prevMag = mag;
		prevPhase = phase;
		csv.line(i*1.0/spectrum.size(), db, phase, groupDelay);
	}
}
