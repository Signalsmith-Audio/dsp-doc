import article
import numpy
from numpy import fft

stepFig, stepAxes = article.small()
figure = article.wideFigure(1, 5, False);
timeAxes = figure.gridPlot((0, 0), (1, 2))
#stepAxes = figure.gridPlot((1, 0))
freqAxes = figure.gridPlot((0, 2), (1, 3));

columns, data = article.readCsv("box-stack-long-time.csv")
for i in range(1, len(columns)):
	timeAxes.plot(data[0]*1.0/len(data[0]), data[i]*len(data[0]), label="%s layers"%columns[i]);
	stepAxes.plot(data[0]*1.0/len(data[0]), numpy.cumsum(data[i]));
timeAxes.set(xlabel="time", ylabel="impulse response (scaled)", xticks=[0, 1], yticks=[0, 1, 2, 3])
stepAxes.set(xlabel="time", ylabel="step response (scaled)")

columns, data = article.readCsv("box-stack-long-freq.csv")
for i in range(1, len(columns)):
	freqAxes.plot(data[0], data[i]);
freqAxes.set(xlim=[0, 10], ylim=[-120, 5], ylabel="dB", xlabel="frequency (relative to filter length)")
figure.save("box-stack-long.svg", legend_loc="upper right")
stepFig.save("box-stack-long-step.svg")

##

columns, data = article.readCsv("box-stack-short-freq.csv")

figure, axes = article.small();
for i in range(1, len(columns)):
	axes.plot(data[0], data[i], label="layers = %s"%columns[i]);
axes.set(xlim=[0, 10], ylim=[-120, 5], ylabel="dB", xlabel="frequency (relative to filter length)")
figure.save("box-stack-short-freq.svg", legend_loc="upper right")

##

columns, data = article.readCsv("box-stack-stats.csv")

figure, axes = article.small();
axes.scatter(data[0], data[1], label="heuristic", color=article.colors[0]);
axes.set(xlim=[0, 10.2], ylim=[0, 18]);
figure.save("box-stack-bandwidth.svg")

figure, axes = article.small();
axes.scatter(data[0], data[2], label="heuristic", color=article.colors[1]);
axes.set(xlim=[0, 10.2], ylim=[-200, 5]);
figure.save("box-stack-peak.svg")

