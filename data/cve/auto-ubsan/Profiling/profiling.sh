#!/bin/bash
#set -e

path="$(pwd)/g/"
testfiles=$(ls $path)
for element in $testfiles
do
export ASAN_OPTIONS=allocator_may_return_null=1:detect_leaks=0:halt_on_error=0
echo $path$element
../autotrace $path$element #&> /dev/null
done