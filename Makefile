all: test

.SILENT: test out/test dev-setup
.PHONY: clean

clean:
	@# Tests and analysis
	rm -rf out
	@# Doxygen
	rm -rf html

CPP_BASE := .
ALL_H := Makefile $(shell find $(CPP_BASE) -iname \*.h)
GCC := g++ -std=c++11 -g -O3 -ffast-math -fno-rtti \
 		-Wall -Wextra -Wfatal-errors -Wpedantic -pedantic-errors \
		-I "util" -I dsp/

# Compile C++ object files
# Everything depends on the .h files in the parent directory
out/%.cpp.o: %.cpp $(ALL_H)
	@echo "$$(tput setaf 3)$<$$(tput sgr0) -> $$(tput setaf 1)$@$$(tput sgr0)"
	@mkdir -p $$(dirname "$@")
	@$(GCC) -c $< -o $@

############## Testing ##############
#
# make test
#	builds all tests (.cpp files) in tests/, excluding directories starting with "_"
#
# make test-foo
#	builds all tests in tests/foo/, excluding directories starting with "_"
#
# make benchmark-foo
#	builds all tests in benchmarks/foo/, excluding directories starting with "_"
#
# For any of these, you can specify SEED=??? in the environment to reproduce randomised tests

# Used for plots and stuff
export PYTHONPATH=$(shell cd util && pwd)

TEST_CPP_FILES := $(shell find tests -iname "*.cpp" -not -path "*/_*/*" | sort)
TEST_CPP_O_FILES := $(patsubst %, out/%.o, $(TEST_CPP_FILES))
test: out/test
	mkdir -p out/analysis
	cd out/analysis && ../test
	cd out/analysis && find ../../tests -iname \*.py -print0 | xargs -0 -n1 python

out/test: out/util/test/main.cpp.o $(TEST_CPP_O_FILES)
	@TEST_OPP_FILES=$$(find out/tests -iname "*.cpp.o" | sort) ;\
	echo "Linking tests:" ;\
	echo "$${TEST_OPP_FILES}" | sed 's/^/     /' ;\
	$(GCC) out/util/test/main.cpp.o $${TEST_OPP_FILES} -o out/test

windows:
	mkdir -p out/windows
	# /O2 for fast code, /Od to disable
	@echo "/Od /Iutil /Idsp /EHsc /Fo\"out/windows/\" /Fe\"out/test.exe\" $(TEST_CPP_FILES)" > tmp.txt

## Individual tests

test-% : out/test-%
	mkdir -p out/analysis
	cd out/analysis && ../test-$*
	cd out/analysis && find ../../tests/$* -iname \*.py -print0 | xargs -0 -n1 python

out/test-%: out/util/test/main.cpp.o
	@# A slight hack: we find the .cpp files, get a list of .o files, and call "make" again
	@TEST_OPP_FILES=$$(find "tests/$*" -iname "*.cpp" | sort | sed "s/\(.*\)/out\/\1.o/") ;\
	$(MAKE) $$TEST_OPP_FILES ;\
	echo "Linking tests:" ;\
	echo "$${TEST_OPP_FILES}" | sed 's/^/     /' ;\
	$(GCC) out/util/test/main.cpp.o $${TEST_OPP_FILES} -o out/test-$*

## Benchmarks

benchmark-% : out/benchmark-%
	mkdir -p out/benchmarks
	cd out/benchmarks && ../benchmark-$*
	cd out/benchmarks && find ../../benchmarks/$* -iname \*.py -print0 | xargs -0 -n1 python

benchmarkpy-%:
	cd out/benchmarks && find ../../benchmarks/$* -iname \*.py -print0 | xargs -0 -n1 python


out/benchmark-%: out/util/test/main.opp
	@TEST_OPP_FILES=$$(find "benchmarks/$*" -iname "*.cpp" | sort | sed "s/\(.*\)/out\/\1.o/") ;\
	make $$TEST_OPP_FILES ;\
	echo "Linking tests:" ;\
	echo "$${TEST_OPP_FILES}" | sed 's/^/     /' ;\
	$(GCC) out/util/test/main.cpp.o $${TEST_OPP_FILES} -o out/benchmark-$*

############## Docs and releases ##############

# These rely on specific things in my dev setup, but you probably don't need to run them yourself

dev-setup:
	#echo "Copying Git hooks (.githooks)"
	#cp .githooks/* .git/hooks

	echo "Adding \"git graph\" and \"git graph-all\"
	git config alias.graph "log --oneline --graph"
	git config alias.graph-all "log --graph --oneline --all"
	cd .. && git config alias.graph "log --oneline --graph"
	cd .. && git config alias.graph-all "log --graph --oneline --all"

historical-docs:
	./historical-docs.sh

release: historical-docs publish publish-git

# bump-patch, bump-minor, bump-major
bump-%: clean all doxygen
	@VERSION=$$(python version.py bump-$*) ; \
		git commit -a -m "Release v$$VERSION" -e && \
		git tag "dev-v$$VERSION" && \
		./git-sub-branch dsp main && \
		git tag "v$$VERSION" main && \
		cp -r html v$$VERSION

release-%: bump-% release
	
doxygen:
	doxygen Doxyfile-local
	cp html/topics.html html/modules.html || echo "Old Doxygen version"

publish:
	find out -iname \*.csv -exec rm {} \;
	find out -iname \*.o -exec rm {} \;
	publish-signalsmith-audio /code/dsp
	cd util && python article

publish-git:
	# Self-hosted
	rm -rf tmp-dsp
	git clone -b main --single-branch . tmp-dsp
	cd tmp-dsp; mv .git/objects/pack/*.pack ./
	cd tmp-dsp; cat *.pack | git unpack-objects
	rm tmp-dsp/.git/refs/tags/dev-*
	cd tmp-dsp; rm -f *.pack && publish-signalsmith-git /code/dsp.git;
	du -sh tmp-dsp/.git
	du -sh .git
	rm -rf tmp-dsp
	git checkout dev && publish-signalsmith-git /code/dsp-doc.git ../dsp/
	# GitHub
	git push github
	git push github main:main
	git push --tags github
	git push release main:main
	git tag | grep "^v[0-9]*\.[0-9]*\.[0-9]*$$" | xargs git push release