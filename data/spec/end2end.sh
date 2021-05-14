echo "Running spec with SanRazor+ASan+L0 >>>"
echo "./run_spec_SR.sh asan L0 $1 &> asan_L0.txt"
./run_spec_SR.sh asan L0 $1 &> asan_L0.txt

echo "Running spec with SanRazor+ASan+L1 >>>"
echo "./run_spec_SR.sh asan L1 $1 &> asan_L1.txt"
./run_spec_SR.sh asan L1 $1 &> asan_L1.txt

echo "Running spec with SanRazor+ASan+L2 >>>"
echo "./run_spec_SR.sh asan L2 $1 &> asan_L2.txt"
./run_spec_SR.sh asan L2 $1 &> asan_L2.txt

echo "Running spec with SanRazor+UBSan+L0 >>>"
echo "./run_spec_SR.sh ubsan L0 $1 &> ubsan_L0.txt"
./run_spec_SR.sh ubsan L0 $1 &> ubsan_L0.txt

echo "Running spec with SanRazor+UBSan+L1 >>>"
echo "./run_spec_SR.sh ubsan L1 $1 &> ubsan_L1.txt"
./run_spec_SR.sh ubsan L1 $1 &> ubsan_L1.txt

echo "Running spec with SanRazor+UBSan+L2 >>>"
echo "./run_spec_SR.sh ubsan L2 $1 &> ubsan_L2.txt"
./run_spec_SR.sh ubsan L2 $1 &> ubsan_L2.txt

echo "Running spec with no sanitizer >>>"
echo "./run_spec_SR.sh ubsan L0 $1 &> default.txt"
./run_spec.sh default $1 &> default.txt

echo "Running spec with ASan >>>"
echo "./run_spec_SR.sh ubsan L1 $1 &> asan.txt"
./run_spec.sh asan $1 &> asan.txt

echo "Running spec with ubSan >>>"
echo "./run_spec_SR.sh ubsan L2 $1 &> ubsan.txt"
./run_spec.sh ubsan $1 &> ubsan.txt

echo "Generate M1/M2 results for spec >>>"
python extract.py --spec_path / --setup SR_asan_L0
python extract.py --spec_path / --setup SR_asan_L1
python extract.py --spec_path / --setup SR_asan_L2
python extract.py --spec_path / --setup SR_ubsan_L0
python extract.py --spec_path / --setup SR_ubsan_L1
python extract.py --spec_path / --setup SR_ubsan_L2