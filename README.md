# DSP Library: Documentation and Tests

This provides tests and doc-generating scripts for Signalsmith Audio's DSP Library.

This should be placed as a sub-directory of the main DSP library (e.g. `doc/`).

## Tests

Tests are in `tests/`, and (where possible) they test the actual audio characteristics of the tools (e.g. frequency responses and aliasing levels).

```
make tests
```

You can compile/run just specific groups of tests, based on the subfolders in `tests/`, e.g.:

```
make test-delay
```

## Plots

Some of the tests write results to CSV files (as well as verifying them).  Any Python files in the `tests/` (sub-)directory are run if the tests succeed, and these generally plot the CSV results into graphs.

Both the tests and the Python scripts are run from the `out/analysis/` directory.

You'll need SciPy/NumPy and Matplotlib, and `ffmpeg` if you want to create some supplemental animations:

```
make analysis-animations
```
