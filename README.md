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

## Quick install & test
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
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j 12
```
3. Run your program with workload. The profiling result will be written into folder `$(pwd)/Cov`.
4. Run the following command to perform sanitizer check reduction:
```
make clean
SanRazor-clang -SR-opt
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j 12
```
5. Test your program after check reduction.

## Reproducing SPEC results
1. Install [SPEC CPU2006 Benchmark](https://www.spec.org/cpu2006/).
2. Run the following code under `SPEC_CPU2006v1.0/` to activate the spec environment:
```
source shrc
```
3. Run the following script to evaluate SanRazor on SPEC CPU2006 Benchmark under `spec/`:
```
./run_spec_SR.sh
```
4. See the evaluation reports under `SPEC_CPU2006v1.0/result`.

## Reproducing CVE results
1. Unzip `Cov.zip` under the source code folder of each software, which contains the coverage information and precompiled LLVM IR files.
2. Run the following command under the source code folder of each software:
```
export SR_STATE_PATH="$(pwd)/Cov"
export SR_WORK_PATH="<path-to-this-file>/coverage.sh"
SanRazor-clang -SR-opt -san-level=<> -use-asap=<>
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j 12
```
3. To reproduce CVE results from scratch (e.g. if the above two steps generate some unexpected errors), you need to firstly unzip `Profiling.zip` under the source code folder of each software, which contains the workload and script for generating coverage information. Then, you can reproduce CVE results following step 1-5 in Section usage of SanRzor. 

## Acknowledgement
We reuse some code from [ASAP](https://github.com/dslab-epfl/asap).
