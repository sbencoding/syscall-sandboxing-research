# container for `chestnut`
# NOTE: run with: `--security-opt seccomp=unconfined` to not interfere with any seccomp stuff the tool tries to attempt during runtime. This is only required if you manage to run a binary built with the tool.
FROM ubuntu:18.04

RUN apt-get update && apt-get -y install \
    vim \
    git \
    cmake \
    ninja-build \
    python3-pip

# Upgrade pip to properly install tool dependencies
RUN python3 -m pip install --upgrade pip

# Get the tool
WORKDIR /tool
RUN git clone https://github.com/IAIK/Chestnut.git
WORKDIR Chestnut/Binalyzer

# install requirements of the binalyzer tool
RUN python3 -m pip install -r requirements.txt

# Sourcealyer stuff
WORKDIR /tool/Chestnut/Sourcealyzer
COPY ./patched_loader.sh /tool/Chestnut/Sourcealyzer/download_and_build.sh
RUN chmod +x ./download_and_build.sh

# Download, patch and build the llvm toolchain
RUN ./download_and_build.sh

# Add llvm build output to path
ENV PATH="${PATH}:/tool/Chestnut/Sourcealyzer/llvm10/build/bin"

# Build the libchestnut binary
COPY ./stage2_loader.sh /tool/Chestnut/Sourcealyzer/stage2_build.sh
RUN chmod +x ./stage2_build.sh
# Install seccomp dependency
RUN apt-get -y install libseccomp-dev
RUN ./stage2_build.sh

# Setup, build and install musl
WORKDIR /tool/musl-build
RUN git clone --depth 1 git://git.musl-libc.org/musl 
WORKDIR musl
RUN ./configure CC="clang -fembed-callhierarchy"
RUN make -j10
RUN make install

# IMPORTANT: Need to run clang with `-fuse-ld=lld` to use a linker which can handle the extra options that the autosandboxing needs
RUN sed -i 's/name=ld/name=ld.lld/g' /usr/local/musl/bin/ld.musl-clang

# Setup, build and install libseccomp using musl
RUN apt-get -y install autoconf libtool gperf
WORKDIR /tool/seccomp
RUN git clone --depth 1 https://github.com/seccomp/libseccomp.git
WORKDIR libseccomp
RUN ./autogen.sh
RUN ln -s /usr/include/linux /usr/local/musl/include/linux
RUN ln -s /usr/include/asm-generic /usr/local/musl/include/asm-generic
RUN ln -s /usr/include/x86_64-linux-gnu/asm/ /usr/local/musl/include/asm
RUN ./configure CC='/usr/local/musl/bin/musl-clang' --prefix='/usr/local/musl'
RUN make -j10 && make install

# Rebuild chestnut.o using the new libseccomp lib
WORKDIR /tool/Chestnut/libchestnut
RUN sed -i 's/clang/\/usr\/local\/musl\/bin\/musl-clang/g' ./Makefile
RUN make clean && make
RUN cp chestnut.o /usr/local/musl/lib

# INFO: -static building results in runtime segfault by the interpreter for some reason
# INFO: note.syscalls shows up when building statically - this is intended by the authors

# Setup musl-clang wrappers that allow static building, because the default ones don't
# when using the `-static` option musl-clang.static should be used to compile
WORKDIR /usr/local/musl/bin
RUN cp ld.musl-clang ld.musl-clang.static
RUN sed -i 's/-lc -dynamic-linker "$ldso"/-l:libc.a -no-dynamic-linker/g' ./ld.musl-clang.static
RUN cp musl-clang musl-clang.static
RUN sed -i 's/musl-clang/musl-clang.static/g' ./musl-clang.static

# Give me shell
CMD bash
