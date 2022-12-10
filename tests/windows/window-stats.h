#include <complex>
#include <cmath>

#include "fft.h"
#include "windows.h"

static double ampToDb(double energy) {
	return 20*std::log10(energy + 1e-100);
}
static double energyToDb(double energy) {
	return 10*std::log10(energy + 1e-100);
}

struct SidelobeStats {
	double mainPeak = 0, sidePeak = 0;
	double mainEnergy = 0, sideEnergy = 0;
	double ratioPeak = 0, ratioEnergy = 0;
	double enbw = 0;
};
template<class Window>
SidelobeStats measureWindow(const Window &windowObj, double bandwidth, bool forcePR=false) {
	SidelobeStats result;
	
	int length = 256;
	int oversample = 64;
	signalsmith::fft::RealFFT<double> realFft(length*oversample);
	std::vector<double> window(length*oversample, 0);

	windowObj.fill(window, length); // Leave the rest as zero padding (oversamples the frequency domain)
	if (forcePR) {
		signalsmith::windows::forcePerfectReconstruction(window, length, length/bandwidth);
	}

	double sum = 0, sum2 = 0;
	for (auto s : window) {
		sum += s;
		sum2 += s*s;
	}
	result.enbw = length*sum2/(sum*sum);

	std::vector<std::complex<double>> spectrum(window.size()/2);
	realFft.fft(window, spectrum);
	
	for (size_t b = 0; b < spectrum.size(); ++b) {
		double freq = b*1.0/oversample;
		double energy = std::norm(spectrum[b]);
		double abs = std::sqrt(energy);
		if (freq <= bandwidth*0.5) {
			result.mainPeak = std::max(abs, result.mainPeak);
			result.mainEnergy += energy;
		} else {
			result.sidePeak = std::max(abs, result.sidePeak);
			result.sideEnergy += energy;
		}
	}
	result.ratioPeak = result.sidePeak/(result.mainPeak + 1e-100);
	result.ratioEnergy = result.sideEnergy/(result.mainEnergy + 1e-100);
	return result;
}
