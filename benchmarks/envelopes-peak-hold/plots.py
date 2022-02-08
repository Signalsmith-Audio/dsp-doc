import article

def plainPlot(name, legend_loc="best"):
	columns, data = article.readCsv("%s.csv"%name)

	figure, axes = article.medium();
	def display(x):
		if x == int(x):
			return str(int(x))
		return str(x)
	xlabels = [display(x) for x in data[0]];
	xticks = range(len(data[0]));

	divisor = int(len(xlabels)*0.2);
	for i in range(len(xlabels)):
		if i%divisor != 0:
			xlabels[i] = ""

	for i in range(1, len(columns)):
		axes.plot(xticks, 1/data[i], label=columns[i]);
	axes.set(ylabel="speed (higher is better)", xticks=xticks, xticklabels=xlabels);
	figure.save("%s.svg"%name, legend_loc=legend_loc)

plainPlot("envelopes_peak_hold_double")
plainPlot("envelopes_peak_hold_float")
plainPlot("envelopes_peak_hold_int")
