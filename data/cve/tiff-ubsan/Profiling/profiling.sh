#!/bin/bash
#set -e

#!/bin/bash
#set -e

cd test
testfiles=$(ls)
for element in $testfiles
do
    if [ ${element:0:8} == "tiffcrop" ]
    then
        $path$element
    fi
done

for element in $testfiles
do
    if [ ${element:0:6} == "tiffcp" ]
    then
        $path$element
    fi
done

for element in $testfiles
do
    if [ ${element:0:8} == "tiff2pdf" ]
    then
        $path$element
    fi
done
cd ../

path="./test/images/"
testfiles=$(ls $path)
for element in $testfiles
do
echo $path$element
./tools/tiffsplit $path$element 
done
