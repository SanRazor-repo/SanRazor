# SanRazor
[![License](https://img.shields.io/github/license/SanRazor-repo/SanRazor?color=blue)](https://opensource.org/licenses/Apache-2.0)
![cmake](https://github.com/SanRazor-repo/SanRazor/workflows/CMake/badge.svg)

SanRazor is a sanitizer check reduction tool aiming to incur little overhead while retaining all important sanitizer checks. 

## Paper
SanRazor: Reducing Redundant Sanitizer Checks in C/C++ Programs. Jiang Zhang, Shuai Wang, Manuel Rigger, Pingjia He, and Zhendong Su. OSDI 2021 (conditional accept)

## Repository structure
1. `src` contains the source code of SanRazor.
2. `var` contains the evaluation results of SanRazor.
3. `data` contains all the information for reproducing the evaluation results of SanRazor.

## Overview
This artifact is supposed to reproduce Figure 4-5 (i.e. runtime performance on SPEC2006) and Table 2 (i.e. CVE Detectability) in this paper.

# 1. Getting Started Instructions
## Quick install & test using docker
```
docker build -f Dockerfile -t sanrazor:latest --shm-size=8g . 
docker run -it sanrazor:latest
bash build_autotrace.sh
```

Note that this docker image is publicly available [here](https://hub.docker.com/r/sanrazor/sanrazor-snapshot), and it contains prebuilt LLVM9 and SanRazor.
# 2. Detailed Instructions
## Prerequisite
```
wget xz-utils cmake make g++ python3 python3-distutils
```
## Install
1. Download and install [LLVM](https://llvm.org/docs/GettingStarted.html) and [Clang](https://clang.llvm.org/get_started.html).
Run the following command in Ubuntu 18.04/20.04 to complete this step:
```
./download_llvm9.sh
```

Note that LLVM9 can not be built on Ubuntu20.04 due to an incompatibility with glibc 2.31 (see [LLVM PR D70662](https://reviews.llvm.org/D70662)). To quickly fix it, you can run the following lines after you download the LLVM9 source code:
```
sed -i 's/unsigned short mode;/unsigned int mode;/g' compiler-rt/lib/sanitizer_common/sanitizer_platform_limits_posix.h
sed -i '/unsigned short __pad1;/d' compiler-rt/lib/sanitizer_common/sanitizer_platform_limits_posix.h
``` 
or 
```
pushd projects/compiler-rt/lib/sanitizer_common
sed -e '1131 s|^|//|' \
    -i sanitizer_platform_limits_posix.cc
popd
```
2. Move the source code of SanRazor into your llvm project:
```
cp -r src/SRPass llvm/lib/Transforms/
cp src/SmallPtrSet.h llvm/include/llvm/ADT/
```
3. Add the following command to `CMakeLists.txt` under `llvm/lib/Transforms`:
```
add_subdirectory(SRPass)
```
4. Compile your llvm project again:
```
./build_and_install_llvm9.sh
```
5. Install [ruby](https://www.ruby-lang.org/en/documentation/installation/) and make sure that the following libraries are installed in your system:
```
gem install fileutils
gem install parallel
gem install pathname
gem install shellwords
```

## Usage of SanRazor
1. Initialization by the following code:
```
export SR_STATE_PATH="$(pwd)/Cov"
export SR_WORK_PATH="<path-to-your-coverage.sh>/coverage.sh"
SanRazor-clang -SR-init
```
2. Set your compiler for C/C++ program as `SanRazor-clang`/`SanRazor-clang++` (`CC=SanRazor-clang`/`CXX=SanRazor-clang++`), and run the following command:
```
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j $(nproc)
```
3. Run your program with workload. The profiling result will be written into folder `$(pwd)/Cov`.
4. Run the following command to perform sanitizer check reduction (Note that we provide the option of using ASAP first with `asap_budget` and running SanRazor later. If you do not want to use ASAP, set `-use-asap=1.0`):
```
make clean
SanRazor-clang -SR-opt -san-level=<L0/L1/L2> -use-asap=<asap_budget>
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j $(nproc)
```
5. Test your program after check reduction.

## Reproducing SPEC results
1. Install [SPEC CPU2006 Benchmark](https://www.spec.org/cpu2006/).
2. Run the following code under `SPEC_CPU2006v1.0/` to activate the spec environment:
```
source shrc
```
3. Run the following script to evaluate SanRazor on SPEC CPU2006 Benchmark under `data/spec/`:
```
./run_spec_SR.sh <asan/ubsan> <L0/L1/L2> <test/ref>
```
4. See the evaluation reports under `SPEC_CPU2006v1.0/result`.

## Reproducing CVE results
1. Unzip `X-Y.tar.gz` to get the source code of software `X` with version `Y`.
2. Compile the source code using clang/gcc to see if there are any errors. Note that sometimes you need to firstly generate Makefile by running the configure script.
3. Unzip `Profiling.zip` under the source code folder of each software, which contains the workload and script for generating coverage information.
4. Compile the source code with `SanRzor-clang`:
```
export SR_STATE_PATH="$(pwd)/Cov"
export SR_WORK_PATH="<path-to-your-coverage.sh>/coverage.sh"
SanRazor-clang -SR-init
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j $(nproc)
```
5. Run `profiling.sh` script in `Profiling` folder. If everything goes well, you will see some text files in `SR_STATE_PATH`, containing the dynamic patterns of checks. 
6. Compile the source code with `SanRazor-clang` again to remove redundant checks:
```
make clean
SanRazor-clang -SR-opt -san-level=<L0/L1/L2> -use-asap=<asap_budget>
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j $(nproc)
```
7. Run `badtest.sh` in `./cve-test` folder to check whether SanRazor can detect these CVEs. Note that test inputs for triggering CVEs are contained in `./cve-test/bad` folder. Some of test inputs in `./cve-test/bad` folder may not be used by `badtest.sh`, since the sanitizer checks protecting these CVE can not be removed by SanRazor. There are two reasons: 1) the sanitizer check can not be identified and removed by SanRazor (e.g. checks in sanitizer_common library); 2) the sanitizer check is not covered during profiling and has no dynamic patterns (i.e. SanRazor will definitely keep it).

## Acknowledgement
We reuse some code from [ASAP](https://github.com/dslab-epfl/asap).
