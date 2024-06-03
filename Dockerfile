# container for my dynamic tracer
FROM ubuntu:18.04

RUN apt-get update && apt-get -y install \
    vim \
    sqlite3 \
    redis-server \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Copy over the interactor and the static tracer
WORKDIR /tracer
COPY ./static_tracer ./tracer
COPY ./interact .

CMD bash
