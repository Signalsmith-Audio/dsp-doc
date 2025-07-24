import numpy

sizes = []
errorRms = []
errorPeak = []

def testSize(size):
	harmonics = list(range(size))
	if size > 100:
		harmonics = numpy.random.choice(harmonics, 20)
	
	errorSum2 = 0
	expectedSum2 = 0
	peak = 0
	
	for harmonic in harmonics:
		phases = ((numpy.arange(size)*harmonic)%size)*2*numpy.pi/size
		input = numpy.cos(phases) + 1j*numpy.sin(phases)
		spectrum = numpy.fft.fft(input)
		
		expected = numpy.zeros(size)
		expected[harmonic] = size
		diff = spectrum - expected
		
		errorSum2 += numpy.average(numpy.abs(diff)**2)
		expectedSum2 += numpy.average(numpy.abs(expected)**2)
		peak = max(peak, max(numpy.abs(diff)))
		
	sizes.append(size)
	eRms = (errorSum2/len(harmonics))**0.5
	xRms = (expectedSum2/len(harmonics))**0.5
	errorRms.append(eRms/xRms)
	errorPeak.append(peak/xRms)

for size in range(1, 16):
	testSize(size)

size = 16
while size <= 2**16:
	testSize(size)
	testSize(size*5//4)
	testSize(size*3//2)
	size *= 2
	
#### Plot the results

import matplotlib
import article

def ampToDb(amp):
	return 20*numpy.log10(numpy.abs(amp) + 1e-100)

figure, axes = article.medium()

axes.plot(sizes, ampToDb(errorPeak), label="peak")
axes.plot(sizes, ampToDb(errorRms), label="RMS")

axes.set(ylabel="aliasing (dB)", xlabel="FFT size", xscale='log', ylim=[-350, 0])

axes.set_xticks([2, 16, 128, 1024, 8192, 65536])
axes.set_xlim([2, None])
axes.get_xaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
axes.get_xaxis().set_tick_params(which='minor', size=0)
axes.get_xaxis().set_tick_params(which='minor', width=0)

figure.save("fft-errors-numpy.svg")

