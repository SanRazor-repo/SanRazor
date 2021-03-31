#!/bin/bash

set -e

# SCRIPT_DIR="$( dirname "$( readlink -f "${BASH_SOURCE[0]}" )" )"
# source "$SCRIPT_DIR/../python/build_utils.sh"

# ALL_C_BENCHMARKS="
#    400.perlbench
#    401.bzip2
#    403.gcc
#    429.mcf
#    445.gobmk
#    456.hmmer
#    458.sjeng
#    462.libquantum
#    464.h264ref
#    433.milc
#    470.lbm
#    482.sphinx3
# "
ALL_BENCHMARKS="
   401.bzip2
   429.mcf
   445.gobmk
   456.hmmer
   458.sjeng
   462.libquantum
   433.milc
   470.lbm
   482.sphinx3
   444.namd
"

#no ubsan h264ref
if ! which runspec > /dev/null; then
    echo "Please run \"source shrc\" in the spec folder prior to calling this script." >&2
    exit 1
fi
export PATH=~/workspace/llvm/build/bin/:$PATH
export SR_WORK_PATH="$(pwd)/coverage.sh"
export ASAN_OPTIONS=alloc_dealloc_mismatch=0:detect_leaks=0:halt_on_error=0
export UBSAN_OPTIONS=halt_on_error=0

runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_$1_$2"  --noreportable --size=$3 ${ALL_BENCHMARKS}

