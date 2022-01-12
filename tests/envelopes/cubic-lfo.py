import article
import numpy
from numpy import fft

columns, data = article.readCsv("cubic-lfo-example.csv")

figure, axes = article.medium();
for i in range(1, len(columns)):
	axes.plot(data[0], data[i], label=columns[i]);
figure.save("cubic-lfo-example.svg", legend_loc="upper center")

####

columns, freqData = article.readCsv("cubic-lfo-spectrum-freq.csv")
columns, depthData = article.readCsv("cubic-lfo-spectrum-depth.csv")

def energyToDb(energy):
	db = 10*numpy.log10(energy + 1e-100)
	db -= max(db)
	return db;

figure, (freqAxes, depthAxes) = article.short(1, 2);
for i in range(1, len(columns)):
	freqAxes.plot(freqData[0], energyToDb(freqData[i]));
	depthAxes.plot(depthData[0], energyToDb(depthData[i]), label=columns[i]);
freqAxes.set(title="frequency variation", ylim=[-65, 5], ylabel="dB (normalised)", xlabel="relative frequency", xlim=[0, 2])
depthAxes.set(title="depth variation", ylim=[-65, 5], ylabel="dB (normalised)", xlabel="relative frequency", xlim=[0, 2])
figure.save("cubic-lfo-spectrum.svg", legend_loc="lower left")

figure, axes = article.small();
for i in [1]:
	axes.plot(freqData[0], energyToDb(freqData[1]), label=columns[i]);
axes.set(ylim=[-80, 5], ylabel="dB (normalised)", xlabel="frequency (relative to LFO speed)", xlim=[0, 10], xticks=range(1, 10, 2))
figure.save("cubic-lfo-spectrum-pure.svg")

###

columns, data = article.readCsv("cubic-lfo-changes.csv")

figure, axes = article.medium();
axes.fill_between(data[0], data[3], data[4], alpha=0.1, color=article.colors[0])
axes.plot(data[0], data[1], label="regular");
axes.plot(data[0], data[2], label="random");
axes.set(yticks=range(-2, 3), xlim=[0, max(data[0])], ylabel="LFO output", xlabel="samples");
figure.save("cubic-lfo-changes.svg", legend_loc="lower center")

####
