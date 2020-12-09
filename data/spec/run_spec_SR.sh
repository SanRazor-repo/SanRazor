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
ALL_C_ASAN_BENCHMARKS="
   400.perlbench
   464.h264ref
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
ALL_C_UBSAN_BENCHMARKS="
   401.bzip2
   429.mcf
   445.gobmk
   456.hmmer
   458.sjeng
   462.libquantum
   433.milc
   470.lbm
   482.sphinx3
"
ALL_CPP_BENCHMARKS="
   471.omnetpp
   473.astar
   483.xalancbmk
   444.namd
   447.dealII
   450.soplex
   453.povray
"
BENCHMARKS="
   464.h264ref
   471.omnetpp
   453.povray
"
TEST="
   410.bwaves
   416.gamess
   434.zeusmp
   435.gromacs
   436.cactusADM
   437.leslie3d
   454.calculix
   459.GemsFDTD
   465.tonto
   481.wrf
"

#no ubsan h264ref
if ! which runspec > /dev/null; then
    echo "Please run \"source shrc\" in the spec folder prior to calling this script." >&2
    exit 1
fi

export SR_WORK_PATH="$(pwd)/coverage.sh"
export ASAN_OPTIONS=alloc_dealloc_mismatch=0:detect_leaks=0:halt_on_error=0
# export UBSAN_OPTIONS=halt_on_error=0
# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_asan_L2"  --noreportable --size=test ${TEST} 
# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_asan_L2"  --noreportable --size=ref ${BENCHMARKS} 

# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_asan_L0"  --noreportable --size=ref 453.povray
runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_ubsan_L0"  --noreportable --size=ref 453.povray
# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_asan_L1"  --noreportable --size=ref 453.povray
runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_ubsan_L1"  --noreportable --size=ref 453.povray
# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_asan_L2"  --noreportable --size=ref 453.povray
# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_ubsan_L2"  --noreportable --size=ref 453.povray
# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension="asan"  --noreportable --size=ref 453.povray
# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension="ubsan"  --noreportable --size=ref 453.povray
# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension=default  --noreportable --size=ref 453.povray
# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="asan"  --noreportable --size=ref 453.povray
# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension=asan  --noreportable --size=ref 453.povray


# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_asan_L2"  --noreportable --size=ref ${ALL_C_ASAN_BENCHMARKS} 
# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_asan_L2"  --noreportable --size=ref ${ALL_CPP_BENCHMARKS}
# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_ubsan_L2"  --noreportable --size=ref ${ALL_CPP_BENCHMARKS}

# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="asan"  --noreportable --size=ref ${ALL_C_ASAN_BENCHMARKS}
# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension=asan  --noreportable --size=ref ${ALL_C_ASAN_BENCHMARKS}

# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="SR_asan"  --noreportable --size=test ${ALL_C_ASAN_BENCHMARKS}
# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension="asan"  --noreportable --size=ref ${ALL_C_ASAN_BENCHMARKS}
# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension=default  --noreportable --size=test ${ALL_C_ASAN_BENCHMARKS}

# runspec --config="$(pwd)/SR_on.cfg" --rebuild --extension="ubsan"  --noreportable --size=ref ${ALL_C_ASAN_BENCHMARKS}
# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension="ubsan"  --noreportable --size=ref ${ALL_C_ASAN_BENCHMARKS}

# runspec --config="$(pwd)/SR_off.cfg" --rebuild --extension=default  --noreportable --size=ref ${ALL_C_ASAN_BENCHMARKS}
