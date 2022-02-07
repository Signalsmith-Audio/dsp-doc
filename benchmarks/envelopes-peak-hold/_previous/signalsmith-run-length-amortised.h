#include <cmath>
#include <vector>
#include <iterator>

namespace signalsmith_run_length_amortised {

	/** Peak-hold filter.
		\diagram{peak-hold.svg}
		
		The size is variable, and can be changed instantly with `.set()`, or by using `.push()`/`.pop()` in an unbalanced way.

		To avoid allocations while running, it uses a fixed-size array (not a `std::deque`) which determines the maximum length.
	*/
	template<typename Sample=double>
	class PeakHold {
		int frontIndex = 0, backIndex = 0;
		int indexMask;
		struct Segment {
			Sample value;
			int length;
		};
		std::vector<Segment> segments;
	public:
		PeakHold(int maxLength) {
			resize(maxLength);
		}
		/// Sets the maximum guaranteed size.
		void resize(int maxLength) {
			int length = 1;
			while (length < maxLength) length *= 2;
			indexMask = length - 1;
			
			frontIndex = backIndex = 0;
			// Sets the size
			segments.assign(length, Segment{0, maxLength});
		}
		/// Calculates the current size
		int size() const {
			int result = 0;
			for (int i = frontIndex; i <= backIndex; ++i) {
				result += segments[i&indexMask].length;
			}
			return result;
		}
		/// Resets the filter, preserving the size
		void reset(Sample fill=Sample()) {
			int s = size();
			frontIndex = backIndex = 0;
			segments[0] = {fill, s};
		}
		
		/// Sets a new size, extending the oldest value if needed
		void set(int newSize) {
			int oldSize = size();
			if (newSize > oldSize) {
				auto &front = segments[frontIndex&indexMask];
				front.length += newSize - oldSize;
			} else {
				for (int i = 0; i < oldSize - newSize; ++i) {
					pop();
				}
			}
		}
		/// Adds a new value and drops an old one.
		Sample operator()(Sample v) {
			pop();
			push(v);
			return read();
		}

		/// Drops the oldest value from the peak tracker.
		void pop() {
			auto &front = segments[frontIndex&indexMask];
			if (--front.length == 0) {
				++frontIndex;
			}
		}
		/// Adds a value to the peak tracker
		void push(Sample v) {
			int length = 1;
			while (backIndex >= frontIndex && segments[backIndex&indexMask].value <= v) {
				// Consume the segment
				length += segments[backIndex&indexMask].length;
				--backIndex;
			}
			++backIndex;
			segments[backIndex&indexMask] = {v, length};
		}
		/// Returns the current value
		Sample read() const {
			return segments[frontIndex&indexMask].value;
		}
	};
}
