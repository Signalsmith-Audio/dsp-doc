from numpy import *
from numpy.fft import rfft

import article

###### STFT windows and partial sums

columns, windowData = article.readCsv("stft-windows.csv")
columns, partialSumData = article.readCsv("stft-windows-partial.csv")

figure, (windowAxes, partialAxes) = article.medium(2)
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

figure.save("stft-windows.svg")

# Simulated aliasing

figure, axes = article.small()

for size in [257, 163, 128, 70]:
	columns, data = article.readCsv("stft-aliasing-simulated-%i.csv"%size)

	overlapRatio = data[0]/data[1]
	axes.plot(overlapRatio, data[2], label="N=%i"%size)

#r = linspace(1, 12, 100)
#axes.plot(r, 11.5 - 14.5*r, label="eyeballed approximation")
#axes.plot(r, 17.5 - 14.5*r, label="eyeballed limit")

axes.set(ylabel="aliasing (dB)", xlabel="overlap ratio (window/interval)", xlim=[1, 12], xticks=range(2, 14, 2), ylim=[-152, 0])
figure.save("stft-aliasing-simulated.svg")
