all: test

.SILENT: test out/test dev-setup check-main-commit update-main-commit
.PHONY: clean

clean:
	@# Tests and analysis
	rm -rf out
	@# Doxygen
	rm -rf html

# Compile C++ object files
# Everything depends on the .h files in the parent directory
out/%.cpp.o: %.cpp ../*.h
	@echo "$$(tput setaf 3)$<$$(tput sgr0) -> $$(tput setaf 1)$@$$(tput sgr0)"
	@mkdir -p $$(dirname "$@")
	@g++ -std=c++11 -Wall -Wextra -Wfatal-errors -g -O3 -ffast-math \
 		-Wpedantic -pedantic-errors \
		-I "util" -I "../" \
		-c $< -o $@

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
	g++ -std=c++11 -Wall -Wextra -Wfatal-errors -g -O3 -ffast-math \
 		-Wpedantic -pedantic-errors \
		out/util/test/main.cpp.o $${TEST_OPP_FILES} \
		-o out/test

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
	g++ -std=c++11 -Wall -Wextra -Wfatal-errors -g -O3 -ffast-math \
 		-Wpedantic -pedantic-errors \
		out/util/test/main.cpp.o $${TEST_OPP_FILES} \
		-o out/test-$*

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
	g++ -std=c++11 -Wall -Wextra -Wfatal-errors -g -O3 -ffast-math \
 		-Wpedantic -pedantic-errors \
		out/util/test/main.cpp.o $${TEST_OPP_FILES} \
		-o out/benchmark-$*

##

check-main-commit:
	CURRENT_COMMIT=$$(cd .. && git log --format="%H" -n 1) ; \
		KNOWN_COMMIT=$$(cat dsp-commit.txt) ; \
		COMMON_ANCESTOR=$$(cd .. && git merge-base "$$KNOWN_COMMIT" "$$CURRENT_COMMIT") ; \
		if [ "$$KNOWN_COMMIT" != "$$CURRENT_COMMIT" ]; then \
			echo "\nUntested commits (make update-main-commit to change):"; \
			cd .. && git log --graph --oneline "$$KNOWN_COMMIT...$$CURRENT_COMMIT"; \
			echo; \
			exit 1; \
		fi;

# Forces you to assert that you've tested all your changes
update-main-commit:
	CURRENT_COMMIT=$$(cd .. && git log --format="%H" -n 1) ; \
		echo "$$CURRENT_COMMIT" > dsp-commit.txt ; \
		git commit dsp-commit.txt -m "Update main library commit" -e

check-git: check-main-commit
	git diff-index --quiet HEAD || (git status && exit 1)
	cd .. && git diff-index --quiet HEAD || (git status && exit 1)

############## Docs and releases ##############

# These rely on specific things in my dev setup, but you probably don't need to run them yourself

dev-setup:
	echo "Copying Git hooks (.githooks)"
	cp .githooks/* .git/hooks

	# From the parent directory, we can run "git both [commands]".
	# The alias starting with "!" means it's handed to bash.  We use "$@" (escaped with $$ because this is a Makefile) to run the commands twice, and end with "#" so that the actual commands are ignored
	echo "Adding \"git both [...]\" alias to current directory"
	git config alias.both "!(cd ..;tput bold;tput smul;tput setaf 4;echo \"============ ./ ============\";tput sgr0; git \"\$$@\" && (cd doc;tput bold;tput smul;tput setaf 3;echo \"============ doc/ ============\";tput sgr0; git \"\$$@\")); #"
	echo "Adding \"git both [...]\" alias to parent directory"
	cd .. && git config alias.both "!(tput bold;tput smul;tput setaf 4;echo \"============ ./ ============\";tput sgr0; git \"\$$@\" && (cd doc;tput bold;tput smul;tput setaf 3;echo \"============ doc/ ============\";tput sgr0; git \"\$$@\")); #"

	# Add "graph" and "graph-all" aliases
	echo "Adding \"git graph\" and \"git graph-all\" to both directories"
	git config alias.graph "log --oneline --graph"
	git config alias.graph-all "log --graph --oneline --all"
	cd .. && git config alias.graph "log --oneline --graph"
	cd .. && git config alias.graph-all "log --graph --oneline --all"

release: check-git clean all doxygen publish publish-git

# bump-patch, bump-minor, bump-major
bump-%: clean all
	@VERSION=$$(python version.py bump-$*) ; \
		pushd .. && git commit -a -m "Release v$$VERSION" -e && git tag "v$$VERSION" ; \
		CURRENT_COMMIT=$$(git log --format="%H" -n 1); \
		popd && echo "$$CURRENT_COMMIT" > dsp-commit.txt ; \
		git commit -a -m "Release v$$VERSION" -e && git tag "v$$VERSION" ;
	
doxygen:
	doxygen Doxyfile-local

publish:
	find out -iname \*.csv -exec rm {} \;
	publish-signalsmith-audio /code/dsp
	cd util && python article

publish-git:
	# Self-hosted
	cd .. && publish-signalsmith-git /code/dsp.git
	publish-signalsmith-git /code/dsp-doc.git ../dsp/
	# GitHub
	cd .. && git push github
	cd .. && git push --tags github
	git push github
	git push --tags github
