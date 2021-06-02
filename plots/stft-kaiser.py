from numpy import *
from numpy.fft import rfft

import article

###### STFT windows and partial sums

columns, windowData = article.readCsv("out/analysis/stft-windows.csv")
columns, partialSumData = article.readCsv("out/analysis/stft-windows-partial.csv")

figure, (windowAxes, partialAxes) = article.medium(2)
figure.set_size_inches(6.5, 6.5)
windowLength = len(windowData[0])
x = windowData[0]/float(windowLength)
for i in range(1, len(columns)):
	interval = float(columns[i])
	ratio = windowLength/interval
	label = "%.1fx (%i/%i)"%(ratio, interval, windowLength)
	windowAxes.plot(x, windowData[i], label=label)
	partialAxes.plot(x, partialSumData[i], label=label)
windowAxes.set(ylabel="synthesis/analysis window")
partialAxes.set(ylabel="effective partial-future window")

figure.save("out/analysis/stft-windows.svg")

###### Kaiser windows with neat ratios, and spectral analysis

def getSpectrum(x, oversample=64):
	padded = concatenate((x, zeros(len(x)*(oversample - 1))))

	spectrum = rfft(padded)
	db = 20*log10(abs(spectrum) + 1e-30)
	db -= db[0]
	bins = arange(len(spectrum))*(1.0/oversample)
	return bins, db

columns, data = article.readCsv("out/analysis/stft-kaiser-windows-neat.csv")

figure, (timeAxes, freqAxes) = article.medium(2)
figure.set_size_inches(6.5, 6.5)
for i in range(1, len(columns)):
	timeAxes.plot(data[0], data[i], label="%sx overlap"%columns[i])
	bins, db = getSpectrum(data[i])
	freqAxes.plot(bins, db)

timeAxes.set(ylim=[-0.1, 1.1])
freqAxes.set(ylim=[-100, 1], xlim=[0, 6], xlabel="bin", ylabel="dB")
figure.save("out/analysis/stft-kaiser-windows-neat.svg")

# Simulated aliasing

figure, axes = article.small()

for size in [257, 163, 128, 70]:
	columns, data = article.readCsv("out/analysis/stft-aliasing-simulated-%i.csv"%size)

	overlapRatio = data[0]/data[1]
	axes.plot(overlapRatio, data[2], label="N=%i"%size)

#r = linspace(1, 12, 100)
#axes.plot(r, 11.5 - 14.5*r, label="eyeballed approximation")
#axes.plot(r, 17.5 - 14.5*r, label="eyeballed limit")

axes.set(ylabel="aliasing (dB)", xlabel="overlap ratio (window/interval)", xlim=[1, 12], xticks=range(2, 14, 2), ylim=[-152, 0])
figure.save("out/analysis/stft-aliasing-simulated.svg")
