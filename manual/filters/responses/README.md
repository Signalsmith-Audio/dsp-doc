# Emscripten build

C++ should include [`wasm-api.h`](wasm-api.h).  This provides some helper methods for passing back arrays and strings, by stashing them in a property on the module.

This can be run from `WasmApi().run(Main, callback) in `wasm-api.js`, which wraps all the exposed functions so that array results and callbacks work.

The `EXPORT_NAME` argument in the `Makefile` should match the `#define` in `main.cpp`.

### Setting up

If you don't already have the Emscripten SDK installed:

```
EMSDK_DIR=<dir> make emsdk emsdk-latest
```

If you already have it, you can make `emsdk-latest` to update.  Then get the environment set up with:

```
EMSDK_DIR=<dir> make emsdk-env
```

### Building

```
EMSDK_DIR=<dir> make out/main.js
```

