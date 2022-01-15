import article
import numpy
from numpy import fft

figure, axes = article.short();

columns, data = article.readCsv("linear-decay.csv")
for i in range(1, len(columns)):
	axes.plot(data[0], data[i], label=columns[i]);
axes.set(xlabel="time")
figure.save("linear-decay.svg", legend_loc="upper center")
