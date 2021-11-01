#include <vector>
#include <complex>

using Spectrum = std::vector<std::complex<double>>;

static double ampToDb(double amp) {
	return 20*std::log10(std::max(amp, 1e-100));
}
static double dbToAmp(double db) {
	return std::pow(10, db*0.05);
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

static double interpSpectrum(Spectrum spectrum, double freq, int roundDirection = 0) {
	double index = freq*spectrum.size();
	int intFreqLow = index;
	int intFreqHigh = intFreqLow + 1;
	if (roundDirection < 0) return std::abs(spectrum[intFreqLow]);
	if (roundDirection > 0) return std::abs(spectrum[intFreqHigh]);
	
	double ratio = index - intFreqLow;
	if (intFreqHigh >= (int)spectrum.size()) {
		--intFreqLow;
		--intFreqHigh;
	}
	double energyLow = std::norm(spectrum[intFreqLow]);
	double energyHigh = std::norm(spectrum[intFreqHigh]);
	return std::sqrt(energyLow + (energyHigh - energyLow)*ratio);
}

static void writeSpectrum(Spectrum spectrum, std::string name) {
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

static bool isMonotonic(const Spectrum &spectrum, double from=0, double to=0.5, double thresholdDb=0.000001) {
	int direction = (from < to) ? 1 : -1;
	if (std::abs(from) >= 1) {
		direction = from;
		from = 0;
		to = 0.5;
	}
	// Monotonically increasing in the specified direction
	int minIndex = std::ceil(std::min(from, to)*spectrum.size());
	int maxIndex = std::floor(std::max(from, to)*spectrum.size());
	int start = (direction > 0) ? minIndex : maxIndex;
	int end = (direction > 0) ? maxIndex + 1 : minIndex - 1;
	
	double thresholdRatio = dbToAmp(-thresholdDb);

	double maxMag = std::abs(spectrum[start]);
	for (int i = start; i != end; i += direction) {
		double mag = std::abs(spectrum[i]);
		if (maxMag > 1e-6 && mag < maxMag*thresholdRatio) { // only care if it's above -120dB
			//std::cout << "not monotonic @" << i << " (" << i*1.0/spectrum.size() << ")\n";
			return false;
		}
		maxMag = std::max(mag, maxMag);
	}
	return true;
}
