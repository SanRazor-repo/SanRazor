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
            hg checkout 3.4
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
    ./build/python -m test -j "$N_JOBS" -x \
        test_aifc test_asyncio test_audioop test_buffer test_cmd_line \
        test_codeccallbacks test_codecs test_ctypes test_datetime test_difflib \
        test_docxmlrpc test_exceptions test_faulthandler test_format test_hash \
        test_httpservers test_io test_itertools test_multiprocessing_fork \
        test_multiprocessing_forkserver test_multiprocessing_spawn test_plistlib \
        test_pyexpat test_signal test_socket test_ssl test_struct test_subprocess \
        test_sunau test_time test_tk test_ttk_guionly test_unicode test_userstring \
        test_xml_etree test_xml_etree_c \
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
build_asap_initial "cpython" "ubsan" "configure_and_build_python" "-O3 -fsanitize=undefined -fno-sanitize-recover=all" "-fsanitize=undefined"

for tool in "asan" "ubsan"; do
    build_asap_coverage "cpython" "$tool" "build_and_test_python"

    build_asap_optimized "cpython" "$tool" "s0000" "-asap-sanity-level=0.000" "build_python"
    build_asap_optimized "cpython" "$tool" "c0010" "-asap-cost-level=0.010" "build_python"
    build_asap_optimized "cpython" "$tool" "c0040" "-asap-cost-level=0.040" "build_python"
    build_asap_optimized "cpython" "$tool" "c1000" "-asap-cost-level=1.000" "build_python"
done


