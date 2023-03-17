#include "./common.h"

#ifndef SIGNALSMITH_DSP_PERF_H
#define SIGNALSMITH_DSP_PERF_H

#include <complex>

#if defined(__SSE__) || defined(_M_X64)
#	include <xmmintrin.h>
#else
#	include <cstdint> // for uintptr_t
#endif

namespace signalsmith {
namespace perf {
	/**	@defgroup Performance Performance helpers
		@brief Nothing serious, just some `#defines` and helpers
		
		@{
		@file
	*/
		
	/// *Really* insist that a function/method is inlined (mostly for performance in DEBUG builds)
	#ifndef SIGNALSMITH_INLINE
	#ifdef __GNUC__
	#define SIGNALSMITH_INLINE __attribute__((always_inline)) inline
	#elif defined(__MSVC__)
	#define SIGNALSMITH_INLINE __forceinline inline
	#else
	#define SIGNALSMITH_INLINE inline
	#endif
	#endif

	/** @brief Complex-multiplication (with optional conjugate second-arg), without handling NaN/Infinity
		The `std::complex` multiplication has edge-cases around NaNs which slow things down and prevent auto-vectorisation.  Flags like `-ffast-math` sort this out anyway, but this helps with Debug builds.
	*/
	template <bool conjugateSecond=false, typename V>
	SIGNALSMITH_INLINE static std::complex<V> mul(const std::complex<V> &a, const std::complex<V> &b) {
		return conjugateSecond ? std::complex<V>{
			b.real()*a.real() + b.imag()*a.imag(),
			b.real()*a.imag() - b.imag()*a.real()
		} : std::complex<V>{
			a.real()*b.real() - a.imag()*b.imag(),
			a.real()*b.imag() + a.imag()*b.real()
		};
	}

#if defined(__SSE__) || defined(_M_X64)
	class NoDenormals {
		unsigned int controlStatusRegister;
	public:
		NoDenormals() : controlStatusRegister(_mm_getcsr()) {
			_mm_setcsr(controlStatusRegister|0x8040); // Flush-to-Zero and Denormals-Are-Zero
		}
		~NoDenormals() {
			_mm_setcsr(controlStatusRegister);
		}
	};
#elif (defined (__ARM_NEON) || defined (__ARM_NEON__))
#	ifdef __arm64__
	class NoDenormals {
		uintptr_t status;
	public:
		NoDenormals() {
			uintptr_t asmStatus;
			asm volatile("vmrs %0, fpscr" : "=r"(asmStatus));
			status = asmStatus = asmStatus|0x01000000U; // Flush to Zero
			asm volatile("vmsr fpscr, %0" : : "ri"(asmStatus));
		}
		~NoDenormals() {
			uintptr_t asmStatus = csr;
			asm volatile("vmsr fpscr, %0" : : "ri"(asmStatus));
		}
	};
#	else
	class NoDenormals {
		uintptr_t status;
	public:
		NoDenormals() {
			uintptr_t asmStatus;
			asm volatile("mrs %0, fpcr" : "=r"(asmStatus));
			status = asmStatus = asmStatus|0x01000000U; // Flush to Zero
			asm volatile("msr fpcr, %0" : : "ri"(asmStatus));
		}
		~NoDenormals() {
			uintptr_t asmStatus = status;
			asm volatile("msr fpcr, %0" : : "ri"(asmStatus));
		}
	};
#	endif
#else
#	if __cplusplus >= 202302L
# 		warning "The `NoDenormals` class doesn't stop denormals with this architecture"
#	endif
	class NoDenormals {}; // FIXME: add for other architectures
#endif

/** @} */
}} // signalsmith::perf::

#endif // include guard
