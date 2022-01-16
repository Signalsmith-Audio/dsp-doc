import article
import numpy
from numpy import fft

for name in ["peak-decay-linear", "peak-decay-linear-cascade"]:
	figure, axes = article.short();
	columns, data = article.readCsv("%s.csv"%name)
	for i in range(1, len(columns)):
		axes.plot(data[0], data[i], label=columns[i]);
	axes.set(xlabel="time", ylim=[0,10])
	figure.save("%s.svg"%name, legend_loc="upper center")
