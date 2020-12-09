#!/bin/bash
#set -e

#!/bin/bash
#set -e

# cd test
# testfiles=$(ls)
# for element in $testfiles
# do
#     if [ ${element:0:8} == "tiffcrop" ]
#     then
#         $path$element
#     fi
# done

# for element in $testfiles
# do
#     if [ ${element:0:6} == "tiffcp" ]
#     then
#         $path$element
#     fi
# done

# for element in $testfiles
# do
#     if [ ${element:0:8} == "tiff2pdf" ]
#     then
#         $path$element
#     fi
# done
# cd ../

path="./test/tif/"
cd ./test/tif
testfiles=$(ls *.tif)
cd ../../
for element in $testfiles
do
export ASAN_OPTIONS=allocator_may_return_null=1:detect_leaks=0:halt_on_error=1
echo $path$element
./tools/tiffcrop -i  $path$element /tmp/foo
./tools/tiffsplit $path$element
./tools/tiffcp -i  $path$element /tmp/foo
# ./tools/tiffcp -c g3  $path$element 
# ./tools/tiffcp -c g4  $path$element 
done
