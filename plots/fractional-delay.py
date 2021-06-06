import numpy
import math
import sys
import os
import glob

import article

def fractionalPlots(prefix):
	figure, (ampAxes, delayAxes) = article.medium(2)
	figure.set_size_inches(6.5, 4.5)
	
	targetDelays = numpy.linspace(0, 0.5, 6)
	
	def limitedCsv(filename):
		columns, data = article.readCsv(filename)
		newColumns = [columns[0]]
		newData = [data[0]]
		
		for target in targetDelays:
			bestIndex = 1
			bestDelay = -1
			for i in range(1, len(columns)):
				delay = float(columns[i])
				diff = abs(delay - target)
				if diff < abs(bestDelay - target):
					bestDelay = delay
					bestIndex = i
			newColumns.append(columns[bestIndex])
			newData.append(data[bestIndex])
	
		return newColumns, numpy.array(newData)
	
	columns, data = limitedCsv(prefix + ".fractional-amplitude.csv")
	for i in range(1, len(columns)):
		ampAxes.plot(data[0], data[i], label=columns[i])
	ampAxes.set(ylim=[-21, 1], ylabel="response (dB)", xlim=[0, 0.5])

	columns, data = limitedCsv(prefix + ".fractional-group.csv")
	minDelay = data[1:,:-1].min()
	maxDelay = data[1:,:-1].max()
	for i in range(1, len(columns)):
		# Nyquist entry is always spurious
		delayAxes.plot(data[0][:-1], data[i][:-1])
	delayAxes.set(ylim=[math.floor(minDelay + 1e-6), math.ceil(maxDelay - 1e-6)], xlim=[0, 0.5], ylabel="samples", xlabel="input frequency (normalised)")

	figure.save(prefix + ".fractional.svg")

def statsPlots(prefix):
	figure, (aliasingAxes, responseAxes, delayAxes) = article.medium(3)
	figure.set_size_inches(6.5, 6.5)
	
	targetDelays = numpy.linspace(0, 0.5, 6)
	
	columns, data = article.readCsv(prefix + ".fractional-stats.csv")
	
	shade_alpha = 0.06

	responseAxes.plot(numpy.concatenate((data[0], [None], data[0])), numpy.concatenate((data[3], [None], data[4])), label="response (range)")
	fillResponse = responseAxes.fill_between(data[0], data[3], data[4], label="delay error (range)", alpha=shade_alpha, color=article.colors[0])
	responseAxes.plot(data[0], data[2], label="response (mean)")
	responseAxes.set(ylim=[-13, 1], ylabel="dB", xlim=[0, 0.5], yticks=[0, -3, -6, -9, -12])
	
	aliasingAxes.plot(data[0], data[1], label="aliasing")
	aliasingAxes.set(ylim=[-95, 0], ylabel="dB", xlim=[0, 0.5])

	delayRangeIndex = min(len(data[5]) - 1, int(len(data[5])*0.95));
	minDelay = data[5,:delayRangeIndex].min()
	maxDelay = data[6:,:delayRangeIndex].max()
	fillDelay = delayAxes.fill_between(data[0][:-1], data[5][:-1], data[6][:-1], label="delay error (range)", alpha=shade_alpha, color=article.colors[0])
	delayAxes.plot(numpy.concatenate((data[0][:-1], [None], data[0][:-1])), numpy.concatenate((data[5][:-1], [None], data[6][:-1])), label="delay error (range)")

	delayAxes.set(ylim=[math.floor(minDelay - 0.1), math.ceil(maxDelay + 0.1)], xlim=[0, 0.5], ylabel="samples", xlabel="input frequency (normalised)")

	figure.save(prefix + ".svg", legend_loc='best')
	figure.save(prefix + "@2x.png", legend_loc='best', dpi=180)
	
def animatePlots(prefix, doubleBack=True):
	columns, impulses = article.readCsv(prefix + ".fractional-impulse.csv")
	columns, amplitudes = article.readCsv(prefix + ".fractional-amplitude.csv")
	columns, delayErrors = article.readCsv(prefix + ".fractional-group.csv")

	impulseLength = 0
	for i in range(len(impulses[1])):
		if max(numpy.abs(impulses[1:,i])) > 1e-4:
			impulseLength = i
	minImpulse = impulses[1:].min()
	
	delayRangeIndex = min(len(delayErrors[0]) - 1, int(len(delayErrors[0])*0.95));
	minDelay = delayErrors[1:,:delayRangeIndex].min()
	maxDelay = delayErrors[1:,:delayRangeIndex].max()
	
	frameStep = 1
	def drawFrame(file, frameNumber, time):
		frameNumber *= frameStep
		if frameNumber > len(columns) - 2:
			frameNumber = (len(columns) - 2)*2 - frameNumber;
		index = min(len(columns) - 1, (frameNumber + 1))
		figure, (impulseAxes, ampAxes, delayAxes) = article.medium(3)
		figure.set_size_inches(6.5, 6.5)
	
		impulseAxes.plot(impulses[0], impulses[index], label="max")
		impulseAxes.set(xlim=[-2, impulseLength + 3], xlabel="sample", ylim=[min(-0.1, minImpulse - 0.1), 1.1], ylabel="impulse response")
		
		ampAxes.plot(amplitudes[0], amplitudes[index])
		ampAxes.set(ylim=[-26, 1], ylabel="dB", xlim=[0, 0.5])

		delayAxes.plot(delayErrors[0][:-1], delayErrors[index][:-1])
		delayAxes.set(ylim=[math.floor(minDelay - 0.1), math.ceil(maxDelay + 0.1)], xlim=[0, 0.5], ylabel="samples", xlabel="input frequency (normalised)")

		figure.save(file, dpi=180)
	
	frameCount = (len(columns) - 1)
	if doubleBack:
		frameCount = 2*frameCount - 2;
	frameCount = int(math.ceil(frameCount*1.0/frameStep))
	article.animate(prefix + ".mp4", drawFrame, fps=30/frameStep, frameCount=frameCount, previewRatio=0.4)

def deleteCsv(prefix):
	os.remove(prefix + ".fractional-amplitude.csv")
	os.remove(prefix + ".fractional-aliasing.csv")
	os.remove(prefix + ".fractional-group.csv")
	os.remove(prefix + ".fractional-impulse.csv")
	# Not currently plotted
	os.remove(prefix + ".fractional-phase.csv")

##########

if len(sys.argv) < 2:
	print("Need prefix or search-dir for the .fractional-???.csv files")
	exit(1)
prefix = sys.argv[1]

tasks = sys.argv[2:]

if os.path.isdir(prefix):
	suffix = ".fractional-amplitude.csv"
	candidates = glob.glob(prefix + "/**" + suffix)
	prefixes = [path[:-len(suffix)] for path in candidates]
	for prefix in prefixes:
		fractionalPlots(prefix)
		statsPlots(prefix)
	if "animate" in tasks:
		for prefix in prefixes:
			animatePlots(prefix)
	if "delete" in tasks:
		for prefix in prefixes:
			deleteCsv(prefix)
else:
	fractionalPlots(prefix)
	statsPlots(prefix)
	if "animate" in tasks:
		animatePlots(prefix)
	if "delete" in tasks:
		deleteCsv(prefix)
