# SanRazor
[![License](https://img.shields.io/github/license/SanRazor-repo/SanRazor)](https://opensource.org/licenses/Apache-2.0)
# Paper submission
Code snapshot for accompanying the manuscript "SanRazor: Reducing Redundant Sanitizer Checks in C/C++ Programs". Our evaluation results can be reproduced from this snapshot. We will provide more documents and instructions to help reproducing the results during the artifact evaluation submission.

## Repository structure
1. `src` contains the source code of SanRazor.
2. `var` contains the evaluation results of SanRazor.
3. `data` contains all the information for reproducing the evaluation results of SanRazor.

Note: this repository refers to https://github.com/dslab-epfl/asap.

## Install
1. Download and install [LLVM](https://llvm.org/docs/GettingStarted.html) and [Clang](https://clang.llvm.org/get_started.html).
We run the following command in Ubuntu 18.04 to complete this step:
```
svn co http://llvm.org/svn/llvm-project/llvm/tags/RELEASE_900/final llvm
cd llvm/tools
svn co http://llvm.org/svn/llvm-project/cfe/tags/RELEASE_900/final clang
cd ..
cd tools/clang/tools
svn co http://llvm.org/svn/llvm-project/clang-tools-extra/tags/RELEASE_900/final extra
cd ../../..
cd projects
svn co http://llvm.org/svn/llvm-project/compiler-rt/tags/RELEASE_900/final compiler-rt
cd ..
cd projects
svn co http://llvm.org/svn/llvm-project/libcxx/tags/RELEASE_900/final libcxx
svn co http://llvm.org/svn/llvm-project/libcxxabi/tags/RELEASE_900/final libcxxabi
cd ..
mkdir bulid
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" ..
make -j 12
sudo make install
```
2. Move the source code of SanRazor into your llvm project:
```
mv src/SRPass llvm/lib/Transforms
```
3. Add the following command to `CMakeLists.txt` under `llvm/lib/Transforms`:
```
add_subdirectory(SRPass)
```
4. Compile your llvm project again:
```
cd llvm/build
make -j 12
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
SanRazor-clang -SR-opt
make CC=SanRazor-clang CXX=SanRazor-clang++ CFLAGS="..." CXXFLAGS="..." LDFLAGS="..." -j 12
```
