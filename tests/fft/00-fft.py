import article
import numpy
import matplotlib

def ampToDb(amp):
	return 20*numpy.log10(numpy.abs(amp) + 1e-100)

figure, (rmsAxes, peakAxes) = article.wide(1, 2)
rmsAxes.set(title="RMS")
peakAxes.set(title="Peak")

def plotType(name, axes, labels=True):
	columns, data = article.readCsv("fft-errors-%s.csv"%name)

	for i in range(1, len(data)):
		label = columns[i] if labels else None
		axes.plot(data[0], ampToDb(data[i]), label=label)

	axes.set(ylabel="aliasing (dB)", xlabel="FFT size", xscale='log', ylim=[-350, 0])

	axes.set_xticks([2, 16, 128, 1024, 8192, 65536])
	axes.set_xlim([2, None])
	axes.get_xaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
	axes.get_xaxis().set_tick_params(which='minor', size=0)
	axes.get_xaxis().set_tick_params(which='minor', width=0)

plotType("rms", rmsAxes, True)
plotType("peak", peakAxes, False)

figure.save("fft-errors.svg", legend_loc='center')
