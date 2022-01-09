import article
import numpy
import matplotlib

columns, data = article.readCsv("fft-errors.csv")

def ampToDb(amp):
	return 20*numpy.log10(numpy.abs(amp) + 1e-100)

figure, axes = article.medium()
for i in range(1, len(data)):
	axes.plot(data[0], ampToDb(data[i]), label=columns[i])

axes.set(ylabel="aliasing (dB)", xlabel="FFT size", xscale='log', ylim=[-350, 0])

axes.set_xticks([2, 16, 128, 1024, 8192, 65536])
axes.set_xlim([2, None])
axes.get_xaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
axes.get_xaxis().set_tick_params(which='minor', size=0)
axes.get_xaxis().set_tick_params(which='minor', width=0)

figure.save("fft-errors.svg")
