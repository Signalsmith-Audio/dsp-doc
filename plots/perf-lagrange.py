from numpy import *
from numpy.fft import rfft

import article

for type in ["double", "float"]:
	columns, data = article.readCsv("out/analysis/performance-lagrange-interpolation-%s.csv"%type)

	figure, axes = article.medium()
	for i in range(1, len(columns)):
		axes.plot(data[0], data[i], label=columns[i])
	axes.set(xlabel="order", ylabel="computation time", ylim=[0, None], xlim=[min(data[0]), max(data[0])], xticks=range(int(min(data[0])), int(max(data[0])) + 1, 2));
	figure.save("out/analysis/performance-lagrange-interpolation-%s"%type)
