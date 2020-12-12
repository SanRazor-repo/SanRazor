FROM sanrazor/sanrazor-snapshot:latest
# COPY src/SRPass /llvm/lib/Transforms/SRPass
COPY data/cve/auto-asan /auto-asan
COPY misc/build_autotrace.sh /build_autotrace.sh

