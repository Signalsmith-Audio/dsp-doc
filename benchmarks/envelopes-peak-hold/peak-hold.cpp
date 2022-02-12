// from the shared library
#include <test/benchmarks.h>

#include "envelopes.h"
#include "_previous/signalsmith-run-length-amortised.h"
#include "_previous/signalsmith-constant-v1.h"

#include "_others/kvr-vadim-zavalishin.h"

template<typename Sample, class PeakHold>
struct PeakHoldImpl {
	int inputSize;
	std::vector<Sample> input, output;
	PeakHold peakHold;
	PeakHoldImpl(int size) : inputSize(size*1000), input(size*1000), output(size*1000), peakHold(size) {}

	inline void run() {
		for (int i = 0; i < inputSize; ++i) {
			output[i] = peakHold(input[i]);
		}
	}
};

template<typename Sample>
struct PeakHoldKvrVadimDynamic {
	int inputSize;
	std::vector<Sample> input, output;
	MovingMaxDynamic<Sample> movingMax;
	PeakHoldKvrVadimDynamic(int size) : inputSize(size*1000), input(size*1000), output(size*1000), movingMax(size) {}

	inline void run() {
		for (int i = 0; i < inputSize; ++i) {
			output[i] = (movingMax << input[i]);
		}
	}
};

template<typename Sample>
void benchmarkPeakHold(Test &test, std::string name) {
	Benchmark<int> benchmark(name, "size");

	benchmark.add<PeakHoldImpl<Sample, signalsmith::envelopes::PeakHold<Sample>>>("current");
	benchmark.add<PeakHoldImpl<Sample, signalsmith_run_length_amortised::PeakHold<Sample>>>("run-length");
	benchmark.add<PeakHoldImpl<Sample, signalsmith_constant_v1::PeakHold<Sample>>>("constant-v1");
	benchmark.add<PeakHoldKvrVadimDynamic<Sample>>("Vadim");

	for (int n = 1; n <= 65536; n *= 2) {
		int n2 = (int(n*0.853253)/2)*2 + 1;
		if (n >= 4) {
			test.log("N = ", n2);
			benchmark.run(n2, n2);
		}

		test.log("N = ", n);
		benchmark.run(n, n);
	}
}

TEST("Peak hold", peak_hold) {
	benchmarkPeakHold<double>(test, "envelopes_peak_hold_double");
	benchmarkPeakHold<float>(test, "envelopes_peak_hold_float");
	benchmarkPeakHold<int>(test, "envelopes_peak_hold_int");
}
