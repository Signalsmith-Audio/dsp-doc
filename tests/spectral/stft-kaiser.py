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
