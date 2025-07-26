#include "./common.h"

#ifndef SIGNALSMITH_DSP_FILTERS_H
#define SIGNALSMITH_DSP_FILTERS_H

#include "./perf.h"

#include <cmath>
#include <complex>

namespace signalsmith {
namespace filters {
	/**	@defgroup Filters Basic filters
		@brief Classes for some common filter types
		
		@{
		@file
	*/
	
	template<class Sample, bool alwaysComplex=false>
	struct Filter2;

	template<class Sample>
	struct ComplexPole {
		using Complex = std::complex<Sample>;

		void operator=(Complex a) {
			a1 = a;
		}

		void reset() {
			state = 0;
		}
		
		Complex operator()(Sample x) {
			return state = state*a1 + x;
		}
		Complex operator()(Complex x) {
			return state = state*a1 + x;
		}
		
		Complex realPair(Sample x) {
			return state = {
				state.real()*a1.real() + x,
				state.imag()*a1.imag() + x
			};
		}

		Complex responseZ(Complex z) const {
			return z/(z - a1);
		}
		Complex responseReal1(Complex z) const {
			return z/(z - a1.real());
		}
		Complex responseReal2(Complex z) const {
			return z/(z - a1.imag());
		}
	private:
		friend struct Filter2<Sample, false>;
		friend struct Filter2<Sample, true>;
		Complex state = 0;
		Complex a1 = 0;
	};
	
	template<class Sample, bool alwaysComplex>
	struct Filter2 {
		using Complex = std::complex<Sample>;
		static constexpr Sample defaultQ = 0.7071067811865476; // sqrt(0.5)
		static constexpr Sample defaultBandwidth = 1.8999686269529916; // equivalent to above Q
		static constexpr Sample maxRefFreq = 0.35;
	
		void reset() {
			pole.reset();
		}
		
		Sample operator()(Sample x) {
			if (alwaysComplex || poleIsComplex) {
				return x*dry + pole(x*gain).real();
			} else {
				auto pair = pole.realPair(x);
				return x*dry + Sample(0.5)*(pair.real()*gain.real() + pair.imag()*gain.imag());
			}
		}
		
		Complex responseZ(Complex z) const {
			if (alwaysComplex || poleIsComplex) {
				return dry
					+ Sample(0.5)*pole.responseZ(z)*gain
					+ Sample(0.5)*std::conj(pole.responseZ(std::conj(z))*gain);
			} else {
				return dry
					+ Sample(0.5)*gain.real()*pole.responseReal1(z)
					+ Sample(0.5)*gain.imag()*pole.responseReal2(z);
			}
		}
		Complex response(Sample f) const {
			return responseZ(std::polar(Sample(1), f*Sample(2*M_PI)));
		}
		Sample responseDb(Sample f) const {
			return 10*std::log10(std::norm(response(f)) + Sample(1e-30));
		}
		
		Filter2 & setPoleZeroGain(Complex p, Complex z, Sample g) {
			poleIsComplex = true;
			pole = p;
			// To get these calculations, rearrange this:
			// 	dry + 0.5*gain/(1 - P*z^-1) + 0.5*conj(gain)/(1 - conj(P)*z^-1)
			// into A(z)/B(z) form, and compare with the classic discrete biquad version
			dry = g*std::norm(z)/std::norm(p);
			Sample gainR = g - dry;
			Sample gainI = (2*g*z.real() - (2*dry + gainR)*p.real())/p.imag();
			gain = {gainR, gainI};
			return *this;
		}
		Filter2 & setPoleCutoffComplex(Complex p, Complex w, Sample gain0, Complex gainW) {
			poleIsComplex = true;
			pole = p;
			Complex A = p;
			Sample H0 = gain0; // Response at f=0, always real
			Sample Hwr = gainW.real(), Hwi = gainW.imag(); // chosen response at cutoff w

			// response at w: dry + gain*X + conj(gain)*Y
			Complex X = Sample(0.5)/(Sample(1) - A*std::conj(w));
			Complex Y = Sample(0.5)/(Sample(1) - std::conj(A)*std::conj(w));
			// response at 1 (f=0): dry + gain*O + conj(gain)*conj(O)
			Complex O = Sample(0.5)/(Sample(1) - A);
			
			// Simultaneous equations from the difference between those two:
			// 	Hwr - H0 = gr*(Xr + Yr - 2*Or) + gi(-Xi + Yi + 2*Oi);
			// 	Hwi = gr*(Xi + Yi) + gi*(Xr - Yr);
			// Phrase as a matrix [[a b] [c d]] * [gr gi]
			Sample mA = X.real() + Y.real() - 2*O.real();
			Sample mB = -X.imag() + Y.imag() + 2*O.imag();
			Sample mC = X.imag() + Y.imag();
			Sample mD = X.real() - Y.real();
			Sample mInvDet = 1/(mA*mD - mB*mC);
			Sample gr = mInvDet*((Hwr - H0)*mD - Hwi*mB);
			Sample gi = mInvDet*((Hwr - H0)*-mC + Hwi*mA);
			// And that's our gain!
			gain = {gr, gi};
			
			dry = H0 - 2*(gr*O.real() - gi*O.imag());
			return *this;
		}
		Filter2 & setPoleCutoffRealPair(Complex p, Complex w, Sample gain0, Complex gainW) {
			poleIsComplex = false; // TODO: if changed, figure out how to map state as our poles turn the corner.  Averaging (real) seems fair - but then is there a way to phrase the real-poles state such that it's already like that
			pole = p;
			Complex A = p;
			Sample H0 = gain0; // Response at f=0, always real
			Sample Hwr = gainW.real(), Hwi = gainW.imag(); // chosen response at cutoff w

			// Two real poles: call them p1/p2, with gains g1/g2
			// response at w: dry + g1*X + g2*Y
			Complex X = Sample(0.5)/(Sample(1) - A.real()*std::conj(w));
			Complex Y = Sample(0.5)/(Sample(1) - A.imag()*std::conj(w));
			// response at 1 (f=0): dry + g1*Or + g2*Oi
			Complex O = {
				Sample(0.5)/(Sample(1) - A.real()),
				Sample(0.5)/(Sample(1) - A.imag())
			};
			
			// Simultaneous equations from the difference between those two:
			// 	Hwr - H0 = g1*(Xr - Or) + g2(Yr - Oi);
			// 	Hwi = g1*Xi + g2*Yi;
			// Phrase as a matrix [[a b] [c d]] * [gr gi]
			Sample mA = X.real() - O.real();
			Sample mB = Y.real() - O.imag();
			Sample mC = X.imag();
			Sample mD = Y.imag();
			Sample mInvDet = 1/(mA*mD - mB*mC);
			Sample gr = mInvDet*((Hwr - H0)*mD - Hwi*mB);
			Sample gi = mInvDet*((Hwr - H0)*-mC + Hwi*mA);
			// And that's our gain!
			gain = {gr, gi};
			
			dry = H0 - (gr*O.real() + gi*O.imag());
			return *this;
		}
		
		Filter2 & lowpassQ(Sample f, Sample q) {
			return fit0cutoff(f, q, 1, [q](Sample iS){
				return Sample(1)/Complex{1 - iS*iS, iS/q};
			});
		}
		Filter2 & highpassQ(Sample f, Sample q) {
			return fit0cutoff(f, q, 0, [q](Sample iS){
				return -iS*iS/Complex{1 - iS*iS, iS/q};
			});
		}
		Filter2 & bandpassQ(Sample f, Sample q) {
			return fit0cutoff(f, q, 0, [q](Sample iS){
				return Complex(0, iS/q)/Complex{1 - iS*iS, iS/q};
			});
		}
		Filter2 & notchQ(Sample f, Sample q) {
			return fit0cutoff(f, q, 1, [q](Sample iS){
				return Complex(1 - iS*iS)/Complex{1 - iS*iS, iS/q};
			});
		}
		Filter2 & peakQ(Sample f, Sample g, Sample q) {
			q *= std::sqrt(g);
			return fit0cutoff(f, q, 1, [q, g](Sample iS){
				return Complex(1 - iS*iS, iS*g/q)/Complex{1 - iS*iS, iS/q};
			});
		}
		Filter2 & highShelfQ(Sample f, Sample g, Sample q) {
			Sample qdrtG = std::sqrt(std::sqrt(g));
			return fit0cutoff(f, q, 1, [q, g](Sample iS){
				Sample A = std::sqrt(g);
				Sample sqrtA = std::sqrt(A);
				return A*Complex{1 - A*iS*iS, sqrtA*iS/q}/Complex{A - iS*iS, sqrtA*iS/q};
			}, qdrtG);
		}
		Filter2 & lowShelfQ(Sample f, Sample g, Sample q) {
			Sample qdrtG = std::sqrt(std::sqrt(g));
			return fit0cutoff(f, q, g, [q, g](Sample iS){
				Sample A = std::sqrt(g);
				Sample sqrtA = std::sqrt(A);
				return A*Complex{A - iS*iS, sqrtA*iS/q}/Complex{1 - A*iS*iS, sqrtA*iS/q};
			}, 1/qdrtG);
		}
		Filter2 & allpassQ(Sample f, Sample q) {
			return fit0cutoff(f, q, 1, [q](Sample iS){
				return Complex(1 - iS*iS, -iS/q)/Complex{1 - iS*iS, iS/q};
			});
		}
	private:
		template<class ResponseFn>
		Filter2 & fit0cutoff(Sample f, Sample q, Sample gain0, ResponseFn &&responseFn, Sample poleScale=1) {
			if (alwaysComplex) q = std::max<Sample>(q, 0.5);
			Sample angF = f*Sample(2*M_PI);
			Sample poleF = angF*poleScale;
			// Pick reference
			Sample iS = maxRefFreq/std::max(maxRefFreq, f);
			Complex refResponse = responseFn(iS);
			Complex refZ = std::polar(Sample(1), angF*iS);

			// Pole taken via IIT
			Sample logAr = -Sample(0.5)/q;
			if (alwaysComplex || logAr >= -1) {
				Sample logAi = std::sqrt(1 - logAr*logAr);
				Complex logA{logAr, logAi};

				Complex A = std::exp(logA*poleF);
				return setPoleCutoffComplex(A, refZ, gain0, refResponse);
			} else {
				Sample logAii = std::sqrt(logAr*logAr - 1);
				Complex logA{logAr - logAii, logAr + logAii};
				
				Complex A = {std::exp(logA.real()*poleF), std::exp(logA.imag()*poleF)};
				return setPoleCutoffRealPair(A, refZ, gain0, refResponse);
			}
			return *this;
		}
	
		ComplexPole<Sample> pole;
		bool poleIsComplex = true;
		Complex gain = 1;
		Sample dry = 0;
	};
	
	/** Filter design methods.
		These differ mostly in how they handle frequency-warping near Nyquist:
		\diagram{filters-lowpass.svg}
		\diagram{filters-highpass.svg}
		\diagram{filters-peak.svg}
		\diagram{filters-bandpass.svg}
		\diagram{filters-notch.svg}
		\diagram{filters-high-shelf.svg}
		\diagram{filters-low-shelf.svg}
		\diagram{filters-allpass.svg}
	*/
	enum class BiquadDesign {
		bilinear, ///< Bilinear transform, adjusting for centre frequency but not bandwidth
 		cookbook, ///< RBJ's "Audio EQ Cookbook".  Based on `bilinear`, adjusting bandwidth (for peak/notch/bandpass) to preserve the ratio between upper/lower boundaries.  This performs oddly near Nyquist.
		oneSided, ///< Based on `bilinear`, adjusting bandwidth to preserve the lower boundary (leaving the upper one loose).
		vicanek ///< From Martin Vicanek's [Matched Second Order Digital Filters](https://vicanek.de/articles/BiquadFits.pdf).  Falls back to `oneSided` for shelf and allpass filters.  This takes the poles from the impulse-invariant approach, and then picks the zeros to create a better match.  This means that Nyquist is not 0dB for peak/notch (or -Inf for lowpass), but it is a decent match to the analogue prototype.
	};
	
	/** A standard biquad.

		This is not guaranteed to be stable if modulated at audio rate.
		
		The default highpass/lowpass bandwidth (`defaultBandwidth`) produces a Butterworth filter when bandwidth-compensation is disabled.
		
		Bandwidth compensation defaults to `BiquadDesign::oneSided` (or `BiquadDesign::cookbook` if `cookbookBandwidth` is enabled) for all filter types aside from highpass/lowpass (which use `BiquadDesign::bilinear`).*/
	template<typename Sample, bool cookbookBandwidth=false>
	class BiquadStatic {
		static constexpr BiquadDesign bwDesign = cookbookBandwidth ? BiquadDesign::cookbook : BiquadDesign::oneSided;
		Sample a1 = 0, a2 = 0, b0 = 1, b1 = 0, b2 = 0;
		Sample x1 = 0, x2 = 0, y1 = 0, y2 = 0;
		
		enum class Type {highpass, lowpass, highShelf, lowShelf, bandpass, notch, peak, allpass};

		struct FreqSpec {
			double scaledFreq;
			double w0, sinW0, cosW0;
			double inv2Q;
			
			FreqSpec(double freq, BiquadDesign design) {
				scaledFreq = std::max(1e-6, std::min(0.4999, freq));
				if (design == BiquadDesign::cookbook) {
					scaledFreq = std::min(0.45, scaledFreq);
				}
				w0 = 2*M_PI*scaledFreq;
				cosW0 = std::cos(w0);
				sinW0 = std::sin(w0);
			}
			
			void oneSidedCompQ() {
				// Ratio between our (digital) lower boundary f1 and centre f0
				double f1Factor = std::sqrt(inv2Q*inv2Q + 1) - inv2Q;
				// Bilinear means discrete-time freq f = continuous-time freq tan(pi*xf/pi)
				double ctF1 = std::tan(M_PI*scaledFreq*f1Factor), invCtF0 = (1 + cosW0)/sinW0;
				double ctF1Factor = ctF1*invCtF0;
				inv2Q = 0.5/ctF1Factor - 0.5*ctF1Factor;
			}
		};
		SIGNALSMITH_INLINE static FreqSpec octaveSpec(double scaledFreq, double octaves, BiquadDesign design) {
			FreqSpec spec(scaledFreq, design);

			if (design == BiquadDesign::cookbook) {
				// Approximately preserves bandwidth between halfway points
				octaves *= spec.w0/spec.sinW0;
			}
			spec.inv2Q = std::sinh(std::log(2)*0.5*octaves); // 1/(2Q)
			if (design == BiquadDesign::oneSided) spec.oneSidedCompQ();
			return spec;
		}
		SIGNALSMITH_INLINE static FreqSpec qSpec(double scaledFreq, double q, BiquadDesign design) {
			FreqSpec spec(scaledFreq, design);

			spec.inv2Q = 0.5/q;
			if (design == BiquadDesign::oneSided) spec.oneSidedCompQ();
			return spec;
		}
		
		SIGNALSMITH_INLINE double dbToSqrtGain(double db) {
			return std::pow(10, db*0.025);
		}
		
		SIGNALSMITH_INLINE BiquadStatic & configure(Type type, FreqSpec calc, double sqrtGain, BiquadDesign design) {
			double w0 = calc.w0;
			
			if (design == BiquadDesign::vicanek) {
				if (type == Type::notch) { // Heuristic for notches near Nyquist
					calc.inv2Q *= (1 - calc.scaledFreq*0.5);
				}
				double Q = (type == Type::peak ? 0.5*sqrtGain : 0.5)/calc.inv2Q;
				double q = (type == Type::peak ? 1/sqrtGain : 1)*calc.inv2Q;
				double expmqw = std::exp(-q*w0);
				double da1, da2;
				if (q <= 1) {
					a1 = da1 = -2*expmqw*std::cos(std::sqrt(1 - q*q)*w0);
				} else {
					a1 = da1 = -2*expmqw*std::cosh(std::sqrt(q*q - 1)*w0);
				}
				a2 = da2 = expmqw*expmqw;
				double sinpd2 = std::sin(w0/2);
				double p0 = 1 - sinpd2*sinpd2, p1 = sinpd2*sinpd2, p2 = 4*p0*p1;
				double A0 = 1 + da1 + da2, A1 = 1 - da1 + da2, A2 = -4*da2;
				A0 *= A0;
				A1 *= A1;
				if (type == Type::lowpass) {
					double R1 = (A0*p0 + A1*p1 + A2*p2)*Q*Q;
					double B0 = A0, B1 = (R1 - B0*p0)/p1;
					b0 = 0.5*(std::sqrt(B0) + std::sqrt(std::max(0.0, B1)));
					b1 = std::sqrt(B0) - b0;
					b2 = 0;
					return *this;
				} else if (type == Type::highpass) {
					b2 = b0 = std::sqrt(A0*p0 + A1*p1 + A2*p2)*Q/(4*p1);
					b1 = -2*b0;
					return *this;
				} else if (type == Type::bandpass) {
					double R1 = A0*p0 + A1*p1 + A2*p2;
					double R2 = -A0 + A1 + 4*(p0 - p1)*A2;
					double B2 = (R1 - R2*p1)/(4*p1*p1);
					double B1 = R2 + 4*(p1 - p0)*B2;
					b1 = -0.5*std::sqrt(std::max(0.0, B1));
					b0 = 0.5*(std::sqrt(std::max(0.0, B2 + 0.25*B1)) - b1);
					b2 = -b0 - b1;
					return *this;
				} else if (type == Type::notch) {
					// The Vicanek paper doesn't cover notches (band-stop), but we know where the zeros should be:
					b0 = 1;
					double db1 = -2*std::cos(w0); // might be higher precision
					b1 = db1;
					b2 = 1;
					// Scale so that B0 == A0 to get 0dB at f=0
					double scale = std::sqrt(A0)/(b0 + db1 + b2);
					b0 *= scale;
					b1 *= scale;
					b2 *= scale;
					return *this;
				} else if (type == Type::peak) {
					double G2 = (sqrtGain*sqrtGain)*(sqrtGain*sqrtGain);
					double R1 = (A0*p0 + A1*p1 + A2*p2)*G2;
					double R2 = (-A0 + A1 + 4*(p0 - p1)*A2)*G2;
					double B0 = A0;
					double B2 = (R1 - R2*p1 - B0)/(4*p1*p1);
					double B1 = R2 + B0 + 4*(p1 - p0)*B2;
					double W = 0.5*(std::sqrt(B0) + std::sqrt(std::max(0.0, B1)));
					b0 = 0.5*(W + std::sqrt(std::max(0.0, W*W + B2)));
					b1 = 0.5*(std::sqrt(B0) - std::sqrt(std::max(0.0, B1)));
					b2 = -B2/(4*b0);
					return *this;
				}
				// All others fall back to `oneSided`
				design = BiquadDesign::oneSided;
				calc.oneSidedCompQ();
			}

			double alpha = calc.sinW0*calc.inv2Q;
			double A = sqrtGain, sqrtA2alpha = 2*std::sqrt(A)*alpha;

			double a0;
			if (type == Type::highpass) {
				b1 = -1 - calc.cosW0;
				b0 = b2 = (1 + calc.cosW0)*0.5;
				a0 = 1 + alpha;
				a1 = -2*calc.cosW0;
				a2 = 1 - alpha;
			} else if (type == Type::lowpass) {
				b1 = 1 - calc.cosW0;
				b0 = b2 = b1*0.5;
				a0 = 1 + alpha;
				a1 = -2*calc.cosW0;
				a2 = 1 - alpha;
			} else if (type == Type::highShelf) {
				b0 = A*((A+1)+(A-1)*calc.cosW0+sqrtA2alpha);
				b2 = A*((A+1)+(A-1)*calc.cosW0-sqrtA2alpha);
				b1 = -2*A*((A-1)+(A+1)*calc.cosW0);
				a0 = (A+1)-(A-1)*calc.cosW0+sqrtA2alpha;
				a2 = (A+1)-(A-1)*calc.cosW0-sqrtA2alpha;
				a1 = 2*((A-1)-(A+1)*calc.cosW0);
			} else if (type == Type::lowShelf) {
				b0 = A*((A+1)-(A-1)*calc.cosW0+sqrtA2alpha);
				b2 = A*((A+1)-(A-1)*calc.cosW0-sqrtA2alpha);
				b1 = 2*A*((A-1)-(A+1)*calc.cosW0);
				a0 = (A+1)+(A-1)*calc.cosW0+sqrtA2alpha;
				a2 = (A+1)+(A-1)*calc.cosW0-sqrtA2alpha;
				a1 = -2*((A-1)+(A+1)*calc.cosW0);
			} else if (type == Type::bandpass) {
				b0 = alpha;
				b1 = 0;
				b2 = -alpha;
				a0 = 1 + alpha;
				a1 = -2*calc.cosW0;
				a2 = 1 - alpha;
			} else if (type == Type::notch) {
				b0 = 1;
				b1 = -2*calc.cosW0;
				b2 = 1;
				a0 = 1 + alpha;
				a1 = b1;
				a2 = 1 - alpha;
			} else if (type == Type::peak) {
				b0 = 1 + alpha*A;
				b1 = -2*calc.cosW0;
				b2 = 1 - alpha*A;
				a0 = 1 + alpha/A;
				a1 = b1;
				a2 = 1 - alpha/A;
			} else if (type == Type::allpass) {
				a0 = b2 = 1 + alpha;
				a1 = b1 = -2*calc.cosW0;
				a2 = b0 = 1 - alpha;
			} else {
				// reset to neutral
				a1 = a2 = b1 = b2 = 0;
				a0 = b0 = 1;
			}
			double invA0 = 1/a0;
			b0 *= invA0;
			b1 *= invA0;
			b2 *= invA0;
			a1 *= invA0;
			a2 *= invA0;
			return *this;
		}
	public:
		static constexpr double defaultQ = 0.7071067811865476; // sqrt(0.5)
		static constexpr double defaultBandwidth = 1.8999686269529916; // equivalent to above Q

		Sample operator ()(Sample x0) {
			Sample y0 = x0*b0 + x1*b1 + x2*b2 - y1*a1 - y2*a2;
			y2 = y1;
			y1 = y0;
			x2 = x1;
			x1 = x0;
			return y0;
		}
		
		void reset() {
			x1 = x2 = y1 = y2 = 0;
		}
		
		void copyFrom(const BiquadStatic &other) {
			b0 = other.b0;
			b1 = other.b1;
			b2 = other.b2;
			a1 = other.a1;
			a2 = other.a2;
		}
		
		std::complex<Sample> response(Sample scaledFreq) const {
			Sample w = scaledFreq*Sample(2*M_PI);
			std::complex<Sample> invZ = {std::cos(w), -std::sin(w)}, invZ2 = invZ*invZ;
			return (b0 + invZ*b1 + invZ2*b2)/(Sample(1) + invZ*a1 + invZ2*a2);
		}
		Sample responseDb(Sample scaledFreq) const {
			Sample w = scaledFreq*Sample(2*M_PI);
			std::complex<Sample> invZ = {std::cos(w), -std::sin(w)}, invZ2 = invZ*invZ;
			Sample energy = std::norm(b0 + invZ*b1 + invZ2*b2)/std::norm(Sample(1) + invZ*a1 + invZ2*a2);
			return 10*std::log10(energy);
		}

		/// @name Lowpass
		/// @{
		BiquadStatic & lowpass(double scaledFreq, double octaves=defaultBandwidth, BiquadDesign design=BiquadDesign::bilinear) {
			return configure(Type::lowpass, octaveSpec(scaledFreq, octaves, design), 0, design);
		}
		BiquadStatic & lowpassQ(double scaledFreq, double q, BiquadDesign design=BiquadDesign::bilinear) {
			return configure(Type::lowpass, qSpec(scaledFreq, q, design), 0, design);
		}
		/// @deprecated use `BiquadDesign` instead
		void lowpass(double scaledFreq, double octaves, bool correctBandwidth) {
			lowpass(scaledFreq, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @deprecated By the time you care about `design`, you should care about the bandwidth
		BiquadStatic & lowpass(double scaledFreq, BiquadDesign design) {
			return lowpass(scaledFreq, defaultBandwidth, design);
		}
		/// @}
		
		/// @name Highpass
		/// @{
		BiquadStatic & highpass(double scaledFreq, double octaves=defaultBandwidth, BiquadDesign design=BiquadDesign::bilinear) {
			return configure(Type::highpass, octaveSpec(scaledFreq, octaves, design), 0, design);
		}
		BiquadStatic & highpassQ(double scaledFreq, double q, BiquadDesign design=BiquadDesign::bilinear) {
			return configure(Type::highpass, qSpec(scaledFreq, q, design), 0, design);
		}
		/// @deprecated use `BiquadDesign` instead
		void highpass(double scaledFreq, double octaves, bool correctBandwidth) {
			highpass(scaledFreq, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @deprecated By the time you care about `design`, you should care about the bandwidth
		BiquadStatic & highpass(double scaledFreq, BiquadDesign design) {
			return highpass(scaledFreq, defaultBandwidth, design);
		}
		/// @}

		/// @name Bandpass
		/// @{
		BiquadStatic & bandpass(double scaledFreq, double octaves=defaultBandwidth, BiquadDesign design=bwDesign) {
			return configure(Type::bandpass, octaveSpec(scaledFreq, octaves, design), 0, design);
		}
		BiquadStatic & bandpassQ(double scaledFreq, double q, BiquadDesign design=bwDesign) {
			return configure(Type::bandpass, qSpec(scaledFreq, q, design), 0, design);
		}
		/// @deprecated use `BiquadDesign` instead
		void bandpass(double scaledFreq, double octaves, bool correctBandwidth) {
			bandpass(scaledFreq, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @deprecated By the time you care about `design`, you should care about the bandwidth
		BiquadStatic & bandpass(double scaledFreq, BiquadDesign design) {
			return bandpass(scaledFreq, defaultBandwidth, design);
		}
		/// @}

		/// @name Notch
		/// @{
		BiquadStatic & notch(double scaledFreq, double octaves=defaultBandwidth, BiquadDesign design=bwDesign) {
			return configure(Type::notch, octaveSpec(scaledFreq, octaves, design), 0, design);
		}
		BiquadStatic & notchQ(double scaledFreq, double q, BiquadDesign design=bwDesign) {
			return configure(Type::notch, qSpec(scaledFreq, q, design), 0, design);
		}
		/// @deprecated use `BiquadDesign` instead
		void notch(double scaledFreq, double octaves, bool correctBandwidth) {
			notch(scaledFreq, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @deprecated By the time you care about `design`, you should care about the bandwidth
		BiquadStatic & notch(double scaledFreq, BiquadDesign design) {
			return notch(scaledFreq, defaultBandwidth, design);
		}
		/// @deprecated alias for `.notch()`
		void bandStop(double scaledFreq, double octaves=1, bool correctBandwidth=true) {
			notch(scaledFreq, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @}

		/// @name Peak
		/// @{
		BiquadStatic & peak(double scaledFreq, double gain, double octaves=1, BiquadDesign design=bwDesign) {
			return configure(Type::peak, octaveSpec(scaledFreq, octaves, design), std::sqrt(gain), design);
		}
		BiquadStatic & peakDb(double scaledFreq, double db, double octaves=1, BiquadDesign design=bwDesign) {
			return configure(Type::peak, octaveSpec(scaledFreq, octaves, design), dbToSqrtGain(db), design);
		}
		BiquadStatic & peakQ(double scaledFreq, double gain, double q, BiquadDesign design=bwDesign) {
			return configure(Type::peak, qSpec(scaledFreq, q, design), std::sqrt(gain), design);
		}
		BiquadStatic & peakDbQ(double scaledFreq, double db, double q, BiquadDesign design=bwDesign) {
			return configure(Type::peak, qSpec(scaledFreq, q, design), dbToSqrtGain(db), design);
		}
		/// @deprecated By the time you care about `design`, you should care about the bandwidth
		BiquadStatic & peak(double scaledFreq, double gain, BiquadDesign design) {
			return peak(scaledFreq, gain, 1, design);
		}
		/// @}

		/// @name High shelf
		/// @{
		BiquadStatic & highShelf(double scaledFreq, double gain, double octaves=defaultBandwidth, BiquadDesign design=bwDesign) {
			return configure(Type::highShelf, octaveSpec(scaledFreq, octaves, design), std::sqrt(gain), design);
		}
		BiquadStatic & highShelfDb(double scaledFreq, double db, double octaves=defaultBandwidth, BiquadDesign design=bwDesign) {
			return configure(Type::highShelf, octaveSpec(scaledFreq, octaves, design), dbToSqrtGain(db), design);
		}
		BiquadStatic & highShelfQ(double scaledFreq, double gain, double q, BiquadDesign design=bwDesign) {
			return configure(Type::highShelf, qSpec(scaledFreq, q, design), std::sqrt(gain), design);
		}
		BiquadStatic & highShelfDbQ(double scaledFreq, double db, double q, BiquadDesign design=bwDesign) {
			return configure(Type::highShelf, qSpec(scaledFreq, q, design), dbToSqrtGain(db), design);
		}
		/// @deprecated use `BiquadDesign` instead
		BiquadStatic & highShelf(double scaledFreq, double gain, double octaves, bool correctBandwidth) {
			return highShelf(scaledFreq, gain, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @deprecated use `BiquadDesign` instead
		BiquadStatic & highShelfDb(double scaledFreq, double db, double octaves, bool correctBandwidth) {
			return highShelfDb(scaledFreq, db, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @}

		/// @name Low shelf
		/// @{
		BiquadStatic & lowShelf(double scaledFreq, double gain, double octaves=2, BiquadDesign design=bwDesign) {
			return configure(Type::lowShelf, octaveSpec(scaledFreq, octaves, design), std::sqrt(gain), design);
		}
		BiquadStatic & lowShelfDb(double scaledFreq, double db, double octaves=2, BiquadDesign design=bwDesign) {
			return configure(Type::lowShelf, octaveSpec(scaledFreq, octaves, design), dbToSqrtGain(db), design);
		}
		BiquadStatic & lowShelfQ(double scaledFreq, double gain, double q, BiquadDesign design=bwDesign) {
			return configure(Type::lowShelf, qSpec(scaledFreq, q, design), std::sqrt(gain), design);
		}
		BiquadStatic & lowShelfDbQ(double scaledFreq, double db, double q, BiquadDesign design=bwDesign) {
			return configure(Type::lowShelf, qSpec(scaledFreq, q, design), dbToSqrtGain(db), design);
		}
		/// @deprecated use `BiquadDesign` instead
		BiquadStatic & lowShelf(double scaledFreq, double gain, double octaves, bool correctBandwidth) {
			return lowShelf(scaledFreq, gain, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @deprecated use `BiquadDesign` instead
		BiquadStatic & lowShelfDb(double scaledFreq, double db, double octaves, bool correctBandwidth) {
			return lowShelfDb(scaledFreq, db, octaves, correctBandwidth ? bwDesign : BiquadDesign::bilinear);
		}
		/// @}
		
		/// @name Allpass
		/// @{
		BiquadStatic & allpass(double scaledFreq, double octaves=1, BiquadDesign design=bwDesign) {
			return configure(Type::allpass, octaveSpec(scaledFreq, octaves, design), 0, design);
		}
		BiquadStatic & allpassQ(double scaledFreq, double q, BiquadDesign design=bwDesign) {
			return configure(Type::allpass, qSpec(scaledFreq, q, design), 0, design);
		}
		/// @}

		BiquadStatic & addGain(double factor) {
			b0 *= factor;
			b1 *= factor;
			b2 *= factor;
			return *this;
		}
		BiquadStatic & addGainDb(double db) {
			return addGain(std::pow(10, db*0.05));
		}
	};

	/** @} */
}} // signalsmith::filters::
#endif // include guard
