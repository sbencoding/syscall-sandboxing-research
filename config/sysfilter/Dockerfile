# container for `sysfilter`
# From last CI config on `main` - extraction/test/docker
FROM ubuntu:18.04

RUN apt-get update && apt-get -y install \
    build-essential \
    libc6-dbg \
    git \
    lcov \
    libreadline-dev \
    gdb \
    lsb-release \
    vim \
    python3-parameterized \
    && rm -rf /var/lib/apt/lists/*

# Fetch the tool from gitlab
WORKDIR /tool
RUN git clone --recursive https://gitlab.com/Egalito/sysfilter.git

# Build the tool
WORKDIR sysfilter/extraction
RUN make -j8 # Sane choice, can edit if needed

# Give me shell
CMD bash
