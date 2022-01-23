// from the shared library
#include <test/benchmarks.h>

#include "fft.h"
#include "_previous/signalsmith-fft-v1.h"

template<typename Sample>
void benchmarkComplex(std::string name) {
	Benchmark<int> benchmark(name, "size");

	struct CreateVectors {
		std::vector<std::complex<Sample>> input, output;
		CreateVectors(int size) : input(size), output(size) {}
	};
	struct Current : CreateVectors {
		signalsmith::fft::FFT<Sample> fft;
		Current(int size) : CreateVectors(size), fft(size) {};
		SIGNALSMITH_INLINE void run() {
			fft.fft(this->input, this->output);
		}
	};
	benchmark.add<Current>("current");

	struct SignalsmithV1 : CreateVectors {
		std::shared_ptr<signalsmith_v1::fft::FFT<Sample>>  fft;
		SignalsmithV1(int size) : CreateVectors(size), fft(signalsmith_v1::fft::getFft<Sample>(size)) {};
		SIGNALSMITH_INLINE void run() {
			fft->fft((Sample *)this->input.data(), (Sample *)this->output.data());
			fft->permute((Sample *)this->input.data(), (Sample *)this->output.data());
		}
	};
	struct SignalsmithV1NoPermute : CreateVectors {
		std::shared_ptr<signalsmith_v1::fft::FFT<Sample>>  fft;
		SignalsmithV1NoPermute(int size) : CreateVectors(size), fft(signalsmith_v1::fft::getFft<Sample>(size)) {};
		SIGNALSMITH_INLINE void run() {
			fft->fft((Sample *)this->input.data(), (Sample *)this->output.data());
		}
	};
	benchmark.add<SignalsmithV1>("signalsmith-v1");
	benchmark.add<SignalsmithV1NoPermute>("signalsmith-v1-nopermute");
	
	for (int n = 1; n <= 65536*16; n *= 2) {
		LOG_EXPR(n);
		benchmark.run(n, std::log2(n)*n + 1);
	}
}

TEST("Complex FFT", complex_fft) {
	benchmarkComplex<double>("complex_fft_double");
}
