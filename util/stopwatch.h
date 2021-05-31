#ifndef SIGNALSMITH_STOPWATCH_UTIL_H
#define SIGNALSMITH_STOPWATCH_UTIL_H

#include <limits>
#include <cmath>
#ifdef WINDOWS // completely untested!
#	include <windows.h>
class Stopwatch {
	using Time = __int64;
	inline Time now() {
		LARGE_INTEGER result;
		QueryPerformanceCounter(&result);
		return result.QuadPart;
	}
#else
#	include <ctime>
class Stopwatch {
	using Time = std::clock_t;
	inline Time now() {
		return std::clock();
	}
#endif

	Time lapStart, lapBest, lapTotal, lapTotal2;
	double lapOverhead;
	int lapCount = 0;
public:
	Stopwatch() {
		start();
		const int repeats = 1000;
		for (int i = 0; i < repeats; ++i) {
			startLap();
			lap();
		}
		lapOverhead = (double)lapTotal/lapCount;
		start();
	}
	
	void start() {
		lapCount = 0;
		lapTotal = lapTotal2 = 0;
		lapBest = std::numeric_limits<Time>::max();
		startLap();
	}
	void startLap() {
		lapStart = now();
	}
	void lap() {
		auto diff = now() - lapStart;

		if (diff < lapBest) lapBest = diff;
		lapCount++;
		lapTotal += diff;
		lapTotal2 += diff*diff;

		startLap();
	}
	double mean() const {
		return (double)lapTotal/lapCount - lapOverhead;
	}
	double var() const {
		double m = (double)lapTotal/lapCount, m2 = (double)lapTotal2/lapCount;
		return std::max(0.0, m2 - m*m);
	}
	double std() const {
		return sqrt(var());
	}
	double best() const {
		return std::max(0.0, lapBest - lapOverhead);
	}
	double optimistic() const {
		return std::max(best(), mean() - std());
	}
};

#endif // include guard
