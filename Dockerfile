FROM sanrazor/sanrazor-snapshot:latest
# COPY src/SRPass /
COPY data/cve/auto-asan /auto-asan
COPY misc/build_autotrace.sh /build_autotrace.sh

