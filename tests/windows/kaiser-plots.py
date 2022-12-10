from numpy import *
from numpy.fft import rfft

import article

###### Kaiser windows with neat ratios, and spectral analysis

def getSpectrum(x, oversample=64):
	padded = concatenate((x, zeros(len(x)*(oversample - 1))))

	spectrum = rfft(padded)
	db = 20*log10(abs(spectrum) + 1e-30)
	db -= db[0]
	bins = arange(len(spectrum))*(1.0/oversample)
	return bins, db

for name in ["kaiser-windows", "kaiser-windows-heuristic", "kaiser-windows-heuristic-pr"]:
	columns, data = article.readCsv("%s.csv"%name)
	figure, (timeAxes, freqAxes) = article.medium(2)
	for i in range(1, len(columns)):
		timeAxes.plot(data[0], data[i], label="%sx bandwidth"%columns[i])
		bins, db = getSpectrum(data[i])
		freqAxes.plot(bins, db)
	timeAxes.set(ylim=[-0.1, 1.1])
	freqAxes.set(ylim=[-100, 1], xlim=[0, 6], xlabel="bin", ylabel="dB")
	figure.save("%s.svg"%name)
