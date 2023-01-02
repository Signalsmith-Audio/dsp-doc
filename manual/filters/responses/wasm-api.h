/**
Usage:
	#define EXPORT_NAME Main
	#include "wasm-api.h"

	extern "C" {
		EXPORT_API
		void string(double value) {
			std::string foo = "foo:" + std::to_string(value);
			heapResultString(foo);
		}
	}

Use `heapResultF64()` when the result will persist until at least the next C++ call.
Use `heapCallbackF64()` when the result is short-lived (e.g. on the stack) and a callback must be used.
 	* F64 -> double
	* F32 -> float
	* U32 -> uint32_t
	* U16 -> uint16_t
	* U8 -> uint8_t
	* 32 -> int32_t
	* 16 -> int16_t
	* 8 -> int8_t

There's also `heapResultString(const char *, int ?length)` and `heapResultString(std::string)`.

Your JavaScript can then call the C++ method (e.g. `Main._myFunc()`), and then call `heapResult()` to get the result, which will be a typed array (referencing live memory) or a string.

If you run this with `WasmApi(Main).run(callback)` from "wasm-api.js", it will wrap your C++ function to handle callbacks or results.
*/

#ifndef EXPORT_NAME
#	error Must define EXPORT_NAME before including "wasm-api.h"
#endif

#include "emscripten.h"

#define EXPORT_API EMSCRIPTEN_KEEPALIVE

#include <cstdint>
#define HEAP_RESULT_TYPED(Module, Type, size, name) \
EM_JS(void, heapResult##name, (const Type *result, int length), { \
	Module.heapResult = Module.HEAP##name.subarray(result/size, result/size + length); \
}); \
EM_JS(void, heapCallback##name, (const Type *result, int length), { \
	if (!Module.heapResultCallback) throw "No callback registered"; \
	Module.heapResultCallback(Module.HEAP##name.subarray(result/size, result/size + length)); \
});
HEAP_RESULT_TYPED(EXPORT_NAME, int8_t, 1, 8);
HEAP_RESULT_TYPED(EXPORT_NAME, int16_t, 2, 16);
HEAP_RESULT_TYPED(EXPORT_NAME, int32_t, 4, 32);
HEAP_RESULT_TYPED(EXPORT_NAME, uint8_t, 1, U8);
HEAP_RESULT_TYPED(EXPORT_NAME, uint16_t, 2, U16);
HEAP_RESULT_TYPED(EXPORT_NAME, uint32_t, 4, U32);
HEAP_RESULT_TYPED(EXPORT_NAME, float, 4, F32);
HEAP_RESULT_TYPED(EXPORT_NAME, double, 8, F64);
// Unpack strings from `const char *`
#include <string>
EM_JS(void, heapResultString, (const char *result, int length), {
	let str = "";
	for (let i = result; i < result + length; ++i) {
		str += String.fromCharCode(Module.HEAP8[i]);
	}
	Module.heapResult = str;
});
void heapResultString(const char *str) {
	heapResultString(str, strlen(str));
}
void heapResultString(const std::string &str) {
	heapResultString(str.c_str(), str.size());
}
