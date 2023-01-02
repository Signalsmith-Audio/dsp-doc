all: out/main.js

.PHONY: all dev-setup emsdk-latest clean

clean:
	rm -rf out

CPP_BASE := .
ALL_H := Makefile $(shell find $(CPP_BASE) -iname \*.h)

EMSDK_DIR ?= $${HOME}/Development/emsdk
emsdk:
	git clone https://github.com/emscripten-core/emsdk.git "$(EMSDK_DIR)"
emsdk-latest:
	cd "$(EMSDK_DIR)" && git pull && ./emsdk install latest && ./emsdk activate latest
emsdk-env:
	. "$(EMSDK_DIR)/emsdk_env.sh" &&\
		python3 --version &&\
		cmake --version

EXPORT_NAME ?= Main
out/%.js: %.cpp $(ALL_H)
	mkdir -p out
	. "$(EMSDK_DIR)/emsdk_env.sh" && emcc \
		$*.cpp -o out/$*.js \
		-I ../../../tests \
		-I ../../.. \
		-std=c++11 -O3 -ffast-math \
		-Wall -Wextra -Wfatal-errors -Wpedantic -pedantic-errors \
		-sSINGLE_FILE=1 -sMODULARIZE \
		-sEXPORT_NAME=$(EXPORT_NAME) \
		-sMEMORY_GROWTH_GEOMETRIC_STEP=0.5 -sALLOW_MEMORY_GROWTH=1

out/main.html: out/main.js
	python3 combine.py index.html out/main.js > out/main.html