#!/bin/bash

set -e

SCRIPT_DIR="$( dirname "$( readlink -f "${BASH_SOURCE[0]}" )" )"
source "$SCRIPT_DIR/build_utils.sh"

fetch_python() {
    if [ -d cpython ]; then
        ( cd cpython && update_repository )
    else
        hg clone http://hg.python.org/cpython
        (
            cd cpython
            hg checkout 2.7
        )
    fi
}

configure_python() {
    cflags=$1
    ldflags=$2

    # We create an extra build folder for python, because python's "make clean"
    # removes all object files under the build folder. This means we can't keep
    # the state folder there.
    mkdir build
    (
        cd build
        ../../cpython/configure \
            --without-pymalloc --disable-shared --disable-ipv6 \
            CC="$(which asap-clang)" \
            CXX="$(which asap-clang++)" \
            AR="$(which asap-ar)" \
            RANLIB="$(which asap-ranlib)" \
            CFLAGS="$cflags" LDFLAGS="$ldflags"
    )
}

build_python() {
    (
        cd build
        make clean
        make -j "$N_JOBS" all
    )
}

test_python() {
    ./build/python -m test.regrtest -j "$N_JOBS" -x \
    test_exceptions test_json test_marshal test_runpy test_ctypes \
    test_descr test_io test_unicode \
        || true
}

configure_and_build_python() {
    configure_python "$@"
    build_python
}

build_and_test_python() {
    build_python
    test_python
}

fetch_python

build_asap_initial "cpython" "baseline" "configure_and_build_python" "-O3" ""
build_asap_initial "cpython" "asan" "configure_and_build_python" "-O3 -fsanitize=address" "-fsanitize=address"


for tool in "asan"; do
    build_asap_coverage "cpython" "$tool" "build_and_test_python"

    build_asap_optimized "cpython" "$tool" "s0000" "-asap-sanity-level=0.000" "build_python"
    build_asap_optimized "cpython" "$tool" "c0010" "-asap-cost-level=0.010" "build_python"
    build_asap_optimized "cpython" "$tool" "c0040" "-asap-cost-level=0.040" "build_python"
    build_asap_optimized "cpython" "$tool" "c1000" "-asap-cost-level=1.000" "build_python"
done





