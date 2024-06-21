# target container for `confine`
FROM ubuntu:18.04

RUN apt-get update && apt-get -y install \
    vim \
    sqlite3 \
    && rm -rf /var/lib/apt/lists/*

CMD /usr/bin/sqlite3
