# SanRazor Artifact
[![License](https://img.shields.io/github/license/SanRazor-repo/SanRazor?color=blue)](https://opensource.org/licenses/Apache-2.0)
![cmake](https://github.com/SanRazor-repo/SanRazor/workflows/CMake/badge.svg)
## Overview
SanRazor is a sanitizer check reduction tool aiming to incur little overhead while retaining all important sanitizer checks. 

## Accepted paper
SanRazor: Reducing Redundant Sanitizer Checks in C/C++ Programs. Jiang Zhang, Shuai Wang, Manuel Rigger, Pingjia He, and Zhendong Su. OSDI 2021 (conditional accept)

## Artifact structure
1. `src` contains the source code of SanRazor.
2. `data` contains all the information for reproducing the evaluation results of SanRazor (SPEC_CPU2006 can be downloaded from [here](https://drive.google.com/file/d/1LAjc3MEfu_Hk3GRmUM8LmfqtUSNS8K5E/view?usp=sharing)).

## Purpose of this artifact
This artifact is supposed to install SanRazor successfully and reproduce Figure 4-5 (i.e. runtime performance on SPEC2006) and Table 2 (i.e. CVE Detectability) in this paper.

## Getting Started Instructions
### 1. Quickly install & test using docker
```
docker build -f Dockerfile -t sanrazor:latest --shm-size=8g . 
docker run -it sanrazor:latest
bash test_autotrace.sh
```

Note that this docker image is publicly available [here](https://hub.docker.com/r/sanrazor/sanrazor-snapshot), and it contains prebuilt LLVM9 and SanRazor. To build it from scratch, you can use `Dockerfile_sanrazor`.
## Detailed Instructions
### 1. Prerequisite
```
wget xz-utils cmake make g++ python3 python3-distutils
```
### 2. Install
2.1. Download and install [LLVM](https://llvm.org/docs/GettingStarted.html) and [Clang](https://clang.llvm.org/get_started.html).
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
pushd llvm/projects/compiler-rt/lib/sanitizer_common
sed -e '1131 s|^|//|' \
    -i sanitizer_platform_limits_posix.cc
popd
```
2.2. Move the source code of SanRazor into your llvm project:
```
cp -r src/SRPass llvm/lib/Transforms/
```
2.3. Run the following command to change `CMakeLists.txt` and `SmallPtrSet.h` (also see `src/patch.sh`):
```
sed -i '7i add_subdirectory(SRPass)' llvm/lib/Transforms/CMakeLists.txt
sed -i "s/static_assert(SmallSize <= .*, \"SmallSize should be small\");/static_assert(SmallSize <= 1024, \"SmallSize should be small\");/g" llvm/include/llvm/ADT/SmallPtrSet.h
```
2.4. Compile your llvm project again:
```
./build_and_install_llvm9.sh
```
2.5. Install [ruby](https://www.ruby-lang.org/en/documentation/installation/) and make sure that the following libraries are installed in your system:
```
gem install fileutils
gem install parallel
gem install pathname
gem install shellwords
```

### 3. Usage of SanRazor
3.1. Initialization by the following code:
```
export SR_STATE_PATH="$(pwd)/Cov"
export SR_WORK_PATH="<path-to-your-coverage.sh>/coverage.sh"
SanRazor-clang -SR-init
```
3.2. Set your compiler for C/C++ program as `SanRazor-clang`/`SanRazor-clang++` (`CC=SanRazor-clang`/`CXX=SanRazor-clang++`), and run the following command:
```
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j $(nproc)
```
3.3. Run your program with workload. The profiling result will be written into folder `$(pwd)/Cov`.
3.4. Run the following command to perform sanitizer check reduction (Note that we provide the option of using ASAP first with `asap_budget` and running SanRazor later. If you do not want to use ASAP, set `-use-asap=1.0`):
```
make clean
SanRazor-clang -SR-opt -san-level=<L0/L1/L2> -use-asap=<asap_budget>
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j $(nproc)
```
3.5. Test your program after check reduction.

### 4. Reproducing SPEC results
4.1. Install [SPEC CPU2006 Benchmark](https://www.spec.org/cpu2006/).
4.2. Run the following code under `SPEC_CPU2006v1.0/` to activate the spec environment:
```
source shrc
```
4.3. Run the following script to evaluate SanRazor on SPEC CPU2006 Benchmark under `data/spec/`:
```
./run_spec_SR.sh <asan/ubsan> <L0/L1/L2> <test/ref>
```
4.4. See the evaluation reports under `SPEC_CPU2006v1.0/result`.

### 5. Reproducing CVE results
5.1. Unzip `X-Y.tar.gz` to get the source code of software `X` with version `Y`.
5.2. Compile the source code using clang/gcc to see if there are any errors. Note that sometimes you need to firstly generate Makefile by running the configure script.
5.3. Unzip `Profiling.zip` under the source code folder of each software, which contains the workload and script for generating coverage information.
5.4. Compile the source code with `SanRzor-clang`:
```
export SR_STATE_PATH="$(pwd)/Cov"
export SR_WORK_PATH="<path-to-your-coverage.sh>/coverage.sh"
SanRazor-clang -SR-init
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j $(nproc)
```
5.5. Run `profiling.sh` script in `Profiling` folder. If everything goes well, you will see some text files in `SR_STATE_PATH`, containing the dynamic patterns of checks. Make sure that you run `profiling.sh` properly and generate the dynmaic patterns of checks before entering into the next step (note that sometimes you need to modify `profiling.sh` the parent directory of the executable profiling program).
5.6. Compile the source code with `SanRazor-clang` again to remove redundant checks:
```
make clean
SanRazor-clang -SR-opt -san-level=<L0/L1/L2> -use-asap=<asap_budget>
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j $(nproc)
```
Please double check whether you set these FLAGS properly. For example, sometimes the `Makefile` may not contain variable like `LDFLAGS` (e.g. `mp3gain`). In this case, you have to revise the `Makefile` a bit and link the ASan/UBSan library properly.

5.7. Run `badtest.sh` in `./cve-test` folder to check whether SanRazor can detect these CVEs. Note that test inputs for triggering CVEs are contained in `./cve-test/bad` folder. Some of test inputs in `./cve-test/bad` folder may not be used by `badtest.sh`, since the sanitizer checks protecting these CVE can not be removed by SanRazor. There are two reasons: 1) the sanitizer check can not be identified and removed by SanRazor (e.g. checks in sanitizer_common library); 2) the sanitizer check is not covered during profiling and has no dynamic patterns (i.e. SanRazor will definitely keep it).

## Acknowledgement
We reuse some code from [ASAP](https://github.com/dslab-epfl/asap).
