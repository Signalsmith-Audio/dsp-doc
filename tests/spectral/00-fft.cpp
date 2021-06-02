#include "spectral.h"

// from the shared library
#include <complex>
#include <cmath>
#include <test/tests.h>

TEST("Windowed FFT: flat window function", no_window) {
	int fftSize = 256;
	
	signalsmith::spectral::WindowedFFT<float> fft;
	fft.setSize(fftSize, [](double) {
		return 1.0;
	});
	
	int harmonic = 5;
	std::vector<float> time(fftSize);
	for (int i = 0; i < fftSize; ++i) {
		double phase = 2*M_PI*(harmonic + 0.5)*i/fftSize;
		time[i] = std::sin(phase)*2;
	}
	std::vector<std::complex<float>> freq(fftSize/2);
	
	fft.fft(time, freq);

	test.closeEnough(freq[harmonic].real(), 0.0f, "real", 1e-4);
	test.closeEnough(freq[harmonic].imag(), -1.0f*fftSize, "imag", 1e-4);
}

double windowHann(double x) {
	return 0.5 - 0.5*std::cos(2*M_PI*x);
}

TEST("Windowed FFT: Hann", windowed_hann) {
	int fftSize = 336; /// 2^4 * 3 * 7 - not a "fast size", but not too slow either
	
	signalsmith::spectral::WindowedFFT<float> fft;
	fft.setSize(fftSize, windowHann, 0);
	
	const std::vector<float> &window = fft.window();
	TEST_ASSERT((int)window.size() == fftSize);
	TEST_ASSERT((int)window.size() == fft.size());
	for (int i = 0; i < fftSize; ++i) {
		float expected = windowHann(i/(float)fftSize);
		TEST_ASSERT(window[i] == expected);
	}
	
	int harmonic = 5;
	std::vector<float> time(fftSize);
	for (int i = 0; i < fftSize; ++i) {
		double phase = 2*M_PI*(harmonic + 0.5)*i/fftSize;
		time[i] = std::cos(phase)*2;
	}
	std::vector<std::complex<float>> freq(fftSize/2);
	
	fft.fft(time, freq);

	test.closeEnough(freq[harmonic - 1].real(), -0.25*fftSize, "n-1", 1e-4);
	test.closeEnough(freq[harmonic].real(), 0.5*fftSize, "n", 1e-4);
	test.closeEnough(freq[harmonic + 1].real(), -0.25*fftSize, "n+1", 1e-4);
}

TEST("Inverse, with windowing", windowed_inverse) {
	int fftSize = 256;
	
	signalsmith::spectral::WindowedFFT<double> fft;
	double offset = 0.25;
	fft.setSize(fftSize, windowHann, offset);
	
	std::vector<double> time(fftSize), output(fftSize);
	for (int i = 0; i < fftSize; ++i) {
		time[i] = test.random(-1.0, 1.0);
	}
	std::vector<std::complex<double>> freq(fftSize/2);
	
	fft.fft(time, freq);
	fft.ifft(freq, output);
	
	for (int i = 0; i < fftSize; ++i) {
		double hann = windowHann((i + offset)/fftSize);
		if (!test.closeEnough(output[i], time[i]*hann*hann, "double-Hann windowed result", 1e-10)) return;
	}
}
