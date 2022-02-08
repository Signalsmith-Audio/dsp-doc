#include <cmath>
#include <vector>
#include <iterator>

#include "delay.h"

namespace signalsmith_constant_v1 {

	/** Peak-hold filter.
		\diagram{peak-hold.svg}
		
		The size is variable, and can be changed instantly with `.set()`, or by using `.push()`/`.pop()` in an unbalanced way.

		To avoid allocations while running, it uses a fixed-size array (not a `std::deque`) which determines the maximum length.
	*/
	template<typename Sample>
	class PeakHold {
		static constexpr double lowest = std::numeric_limits<double>::lowest();
		signalsmith::delay::Buffer<Sample> buffer;
		int backIndex = 0, middleStart = 0, workingIndex = 0, middleEnd = 0, frontIndex = 0;
		Sample frontMax = lowest, workingMax = lowest, middleMax = lowest;
		
	public:
		PeakHold(int maxLength) : buffer(maxLength) {
			backIndex = -maxLength; // use as starting length as well
			reset();
		}
		int size() {
			return frontIndex - backIndex;
		}
		void resize(int maxLength) {
			buffer.resize(maxLength);
			frontIndex = 0;
			backIndex = -maxLength;
			reset();
		}
		void reset(Sample fill=lowest) {
			int prevSize = size();
			buffer.reset(fill);
			frontMax = workingMax = middleMax = lowest;
			middleEnd = workingIndex = frontIndex = 0;
			middleStart = middleEnd - (prevSize/2);
			backIndex = frontIndex - prevSize;
			if (middleStart == backIndex) ++middleStart; // size-0 case
		}
		void set(int newSize) {
			while (size() < newSize) {
				push(frontMax);
			}
			while (size() > newSize) {
				pop();
			}
		}
		
		void push(Sample v) {
			buffer[frontIndex] = v;
			++frontIndex;
			frontMax = std::max(frontMax, v);
		}
		void pop() {
			if (backIndex == middleStart) {
				// Move along the maximums
				workingMax = lowest;
				middleMax = frontMax;
				frontMax = lowest;

				int prevFrontLength = frontIndex - middleEnd;
				int prevMiddleLength = middleEnd - middleStart;
				if (prevFrontLength <= prevMiddleLength) {
					// Swap over simply
					middleStart = middleEnd;
					middleEnd = frontIndex;
					workingIndex = middleEnd;
				} else {
					// The front is longer than expected - this only happens when we're changing size
					// We don't move *all* of the front over, keeping half the surplus in the front
					int middleLength = (frontIndex - middleStart)/2;
					middleStart = middleEnd;
					if (middleStart == backIndex) ++middleStart;
					middleEnd += middleLength;
					// Since the front was not completely consumed, we re-calculate the front's maximum
					for (int i = middleEnd; i != frontIndex; ++i) {
						frontMax = std::max(frontMax, buffer[i]);
					}

					// Working index is close enough that it will be finished by the time the back is empty
					int backLength = middleStart - backIndex;
					int workingLength = std::min(backLength - 1, middleEnd - middleStart);
					workingIndex = middleStart + workingLength;
					// The index might not start at the end of the working block - compute the last bit immediately
					for (int i = middleEnd - 1; i != workingIndex - 1; --i) {
						buffer[i] = workingMax = std::max(workingMax, buffer[i]);
					}
				}

			}

			++backIndex;
			if (workingIndex != middleStart) {
				--workingIndex;
				buffer[workingIndex] = workingMax = std::max(workingMax, buffer[workingIndex]);
			}
		}
		Sample read() {
			Sample backMax = buffer[backIndex];
			return std::max(backMax, std::max(middleMax, frontMax));
		}
		
		// For simple use as a constant-length filter
		Sample operator ()(Sample v) {
			push(v);
			pop();
			return read();
		}
	};
}
