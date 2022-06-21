# Roadmap

## v1.2

General:
	* check the things from that email

Delay:
	* block-based copying
	
Filters:
	* all basic filter types, for at least the default mode
		* What's missing?  Allpass?
	* test shelves
	* test dB spec

Mix:
	* test everything

Windows:
	* Approximate Confined Gaussian
		* map to/from s.d. in time and frequency vs its internal parameter

## v2

General:
	* reorganise as submodule, getting rid of the weird `dsp-commit.txt` hack
	* change to Boost license
	* separate `PLOT(...)` macro?
	* move more things to C++ plots

Delay:
	* block processing and copying
	* value type separate from time-type (in Reader and Interpolator)
	* better views

Filters:
	* finish Vicanek: https://vicanek.de/articles/BiquadFits.pdf
	* remove `cookbookBandwidth` and deprecated methods
	* specify with Q as well as bandwidth
	* (?) Orfandis (specific types, or general solution)

Curves:
	* Arbitrary polynomial
		* differentiation
		* Hermite
	* Arbitrary polynomial-segment curves
	* Arbitrary LFO shape
	* Polynomial segments with min-phase BLEP-y stuff

FFT:
	* benchmarks
	* partial evaluation
