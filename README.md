# SanRazor
[![License](https://img.shields.io/github/license/SanRazor-repo/SanRazor?color=blue)](https://opensource.org/licenses/Apache-2.0)
![cmake](https://github.com/SanRazor-repo/SanRazor/workflows/CMake/badge.svg)

SanRazor is a sanitizer check reduction tool aiming to incur little overhead while retaining all important sanitizer checks.
# Paper submission
The code snapshot for accompanying the manuscript "SanRazor: Reducing Redundant Sanitizer Checks in C/C++ Programs". Our evaluation results can be reproduced from this snapshot. We will provide more documents and instructions to help reproducing the results during the artifact evaluation submission.

## Repository structure
1. `src` contains the source code of SanRazor.
2. `var` contains the evaluation results of SanRazor.
3. `data` contains all the information for reproducing the evaluation results of SanRazor.

## Quick install & test using docker
```
docker build -f Dockerfile -t sanrazor:latest --shm-size=8g . 
docker run -it sanrazor:latest
bash build_autotrace.sh
```

## Install
1. Download and install [LLVM](https://llvm.org/docs/GettingStarted.html) and [Clang](https://clang.llvm.org/get_started.html).
Run the following command in Ubuntu 18.04/20.04 to complete this step:
```
bash download_llvm9.sh
```
2. Move the source code of SanRazor into your llvm project:
```
cp -r SanRazor/src/SRPass llvm/lib/Transforms/
cp SanRazor/src/SmallPtrSet.h llvm/include/llvm/ADT/
```
3. Add the following command to `CMakeLists.txt` under `llvm/lib/Transforms`:
```
add_subdirectory(SRPass)
```
4. Compile your llvm project again:
```
cd llvm/build
make -j 12
sudo make install
```
5. Install [ruby](https://www.ruby-lang.org/en/documentation/installation/) and make sure that the following libraries are installed in your system:
```
fileutils
parallel
pathname
shellwords
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
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j 8
```
3. Run your program with workload. The profiling result will be written into folder `$(pwd)/Cov`.
4. Run the following command to perform sanitizer check reduction:
```
make clean
SanRazor-clang -SR-opt -san-level=<L0/L1/L2> -use-asap=<asap_budget>
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j 8
```
Note that we provide the option of using ASAP first with `asap_budget` and running SanRazor later. If you do not want to use ASAP, set `-use-asap=1.0`.
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
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j 8
```
5. Run `profiling.sh` script in `Profiling` folder. If everything goes well, you will see some text files in `SR_STATE_PATH`, containing the dynamic patterns of checks. 
6. Compile the source code with `SanRazor-clang` again to remove redundant checks:
```
make clean
SanRazor-clang -SR-opt -san-level=<L0/L1/L2> -use-asap=<asap_budget>
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j 8
```
7. Run `badtest.sh` in `./cve-test` folder to check whether SanRazor can detect these CVEs. Note that test inputs for triggering CVEs are contained in `./cve-test/bad` folder.

## Acknowledgement
We reuse some code from [ASAP](https://github.com/dslab-epfl/asap).
