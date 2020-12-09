#!/bin/bash

set -e

export ASAN_OPTIONS="malloc_context_size=0:detect_leaks=0"

PERF_PY=benchmarks-9a1136898539/perf.py

if ! [ -f "$PERF_PY" ]; then
    echo "cannot find $PERF_PY" >&2
    exit 1
fi

python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-asan-initial-build/python
python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-asan-s0000-build/python
python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-asan-c0010-build/python
python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-asan-c0040-build/python
python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-asan-c1000-build/python

python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-ubsan-initial-build/python
python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-ubsan-s0000-build/python
python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-ubsan-c0010-build/python
python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-ubsan-c0040-build/python
python "$PERF_PY" -v "$@" ./cpython-baseline-build/python ./cpython-ubsan-c1000-build/python
