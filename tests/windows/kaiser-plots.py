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

# Bandwidth : sidelobes
columns, data = article.readCsv("kaiser-bandwidth-sidelobes.csv")
mainFigure, mainAxes = article.small()
heuristicFigure, heuristicAxes = article.small()

mainAxes.plot(data[0], data[4], label="peak ratio")
mainAxes.plot(data[0], data[5], label="energy ratio")
heuristicAxes.plot(data[0], data[4] - data[1], label="peak")
heuristicAxes.plot(data[0], data[5] - data[2], label="energy")

mainAxes.set(ylabel="side/main lobes (dB)", xlabel="bandwidth", xlim=[0,None])
mainFigure.save("kaiser-bandwidth-sidelobes.svg")
heuristicAxes.set(ylabel="heuristic difference (dB)", xlabel="bandwidth", xlim=[0,None])
heuristicFigure.save("kaiser-heuristic-improvements.svg")

### For tuning the bandwidth -> dB approximations
#def f(b): # exact peak
#	b = b + (b < 2)*(2 - b)*0
#	return 8/(b + 2) - 12.75*b + 10 + (b < 2)*4*(b - 2)
#def f2(b): # exact energy
#	return 15/(b + 0.4) - 13.25*b + 10.5 + (b < 2)*13*(b - 2)
#def f2h(b): # heuristic-optimal energy
#	b = b + (b < 3)*(3 - b)*0.5
#	return -3/(b + 0.4) - 13.4*b + 12.9 + (b < 3)*-9.6*(b - 3)
#approxFigure, approxAxes = article.medium();
#approxAxes.plot(data[0], data[1] - f(data[0]), label="peak error")
#approxAxes.plot(data[0], data[2] - f2(data[0]), label="energy error")
#approxAxes.plot(data[0], data[5] - f2h(data[0]), label="heuristic error")
#approxAxes.set(ylabel="dB", xlabel="bandwidth")
#approxFigure.save("kaiser-approx-tuning.svg")

## Plot the optimal bandwidth - enable this (and brute-force searching in the "stft_kaiser_bandwidth_sidelobes" test) to tune the heuristic
#figure, axes = article.medium()
#axes.plot(data[0], data[3], label="optimal")
#def h(b):
#	return b + 8/(b + 3)**2 + 0.3*maximum(3 - b, 0);
#axes.plot(data[0], h(data[0]), label="heuristic")
#figure.save("kaiser-heuristic-tuning.svg")
