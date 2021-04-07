#!/bin/bash
sed -i 's/unsigned short mode;/unsigned int mode;/g' llvm/projects/compiler-rt/lib/sanitizer_common/sanitizer_platform_limits_posix.h
sed -i '/unsigned short __pad1;/d' llvm/projects/compiler-rt/lib/sanitizer_common/sanitizer_platform_limits_posix.h
# pushd llvm/projects/compiler-rt/lib/sanitizer_common
# sed -e '1131 s|^|//|' \
#     -i sanitizer_platform_limits_posix.cc
# popd
sed -i '7i add_subdirectory(SRPass)' llvm/lib/Transforms/CMakeLists.txt
sed -i "s/static_assert(SmallSize <= .*, \"SmallSize should be small\");/static_assert(SmallSize <= 1024, \"SmallSize should be small\");/g" llvm/include/llvm/ADT/SmallPtrSet.h