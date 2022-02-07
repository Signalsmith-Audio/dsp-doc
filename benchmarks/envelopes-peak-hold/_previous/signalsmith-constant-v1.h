#include <cmath>
#include <vector>
#include <iterator>

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
			frontIndex = maxLength; // use as starting length as well
			reset();
		}
		int size() {
			return frontIndex - backIndex;
		}
		void reset(Sample fill=lowest) {
			int prevSize = size();
			buffer.reset(fill);
			frontMax = workingMax = middleMax = lowest;
			backIndex = 0;
			middleEnd = workingIndex = frontIndex = prevSize;
			middleStart = prevSize/2;
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
			++backIndex;
			--workingIndex;
			buffer[workingIndex] = workingMax = std::max(workingMax, buffer[workingIndex]);

			if (backIndex >= workingIndex) {
				int newMiddleSize = (frontIndex + middleStart)/2 - middleStart;
				middleStart = middleEnd;
				middleEnd = std::min(middleStart + newMiddleSize, frontIndex);

				middleMax = frontMax;
				frontMax = lowest;
				for (int i = middleEnd; i != frontIndex; ++i) {
					frontMax = std::max(frontMax, buffer[i]);
				}
				workingIndex = std::min(middleStart + middleStart - backIndex, middleEnd);
				workingMax = lowest;
				for (int i = middleEnd; i != workingIndex; --i) {
					buffer[i - 1] = workingMax = std::max(workingMax, buffer[i - 1]);
				}
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
