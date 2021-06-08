all: test analysis-plots

clean:
	# Tests and analysis
	rm -rf out
	# Doxygen
	rm -rf html

############## Testing ##############

test: out/test
	mkdir -p out/analysis
	cd out && ./test

out/test: $(shell find .. -iname "*.h") $(shell find tests -iname "*.cpp")
	TEST_CPP_FILES=$$(find tests -iname "*.cpp" | sort) ;\
	echo "building tests: $${TEST_CPP_FILES}" ;\
	mkdir -p out ;\
	g++ -std=c++11 -Wall -Wextra -Wfatal-errors -g -O3 -ffast-math \
 		-Wpedantic -pedantic-errors \
		"util/test/main.cpp" -I "util" \
		-I tests/ $${TEST_CPP_FILES} \
		-I "../" -I "signalsmith-fft/" \
		-DANALYSIS_CSV_PREFIX="\"analysis/\"" \
		-o out/test

# Make a particular sub-directory in tests/
test-% : out/test-%
	mkdir -p out/analysis
	cd out && ./test-$* --seed=1

out/test-%: $(shell find .. -iname "*.h") $(shell find tests -iname "*.cpp")
	TEST_CPP_FILES=$$(find tests/$* -iname "*.cpp" | sort) ;\
	echo "building tests: $${TEST_CPP_FILES}" ;\
	mkdir -p out ;\
	g++ -std=c++11 -Wall -Wextra -Wfatal-errors -g -O3 -ffast-math \
 		-Wpedantic -pedantic-errors \
		"util/test/main.cpp" -I "util" \
		-I tests/ $${TEST_CPP_FILES} \
		-I "../" -I "signalsmith-fft/" \
		-DANALYSIS_CSV_PREFIX="\"analysis/\"" \
		-o out/test-$*

analysis-plots:
	python plots/stft-kaiser.py
	python plots/perf-lagrange.py
	python plots/fractional-delay.py out/analysis

analysis-animations:
	python plots/fractional-delay.py out/analysis animate

############## Docs and releases ##############

# These rely on specific things in my dev setup, but you probably don't need to run them yourself

release: check-git clean all doxygen publish publish-git

check-git:
	git diff-index --quiet HEAD || (git status && exit 1)
	cd .. && git diff-index --quiet HEAD || (git status && exit 1)

doxygen:
	doxygen

publish:
	find out -iname \*\@2x.png -exec rm {} \;
	find out -iname \*.csv -exec rm {} \;
	publish-signalsmith-audio /code/dsp

publish-git:
	cd plots && python article
	# Self-hosted
	cd .. && publish-signalsmith-git /code/dsp.git
	publish-signalsmith-git /code/dsp-doc.git ../dsp/
	# GitHub
	cd .. && git push --all github
	git push --all github
