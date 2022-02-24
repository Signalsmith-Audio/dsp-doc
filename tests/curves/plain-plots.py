import article
import numpy
from numpy import fft

def plainPlot(name, legend_loc="best", ylim=[None, None]):
	figure, axes = article.short();
	columns, data = article.readCsv("%s.csv"%name)
	for i in range(1, len(columns)):
		axes.plot(data[0], data[i], label=columns[i]);
	axes.set(xlabel="time", ylim=ylim)
	figure.save("%s.svg"%name, legend_loc=legend_loc)

plainPlot("cubic-segments-example")
