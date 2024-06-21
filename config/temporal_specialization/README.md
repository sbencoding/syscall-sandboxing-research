# Temporal Specialization
Setup and usage for the `temporal-specialization` project (tested on commit hash 7c905d3d562e42f51efec606b28fdd07fdee2c0e).
Source: https://github.com/shamedgh/temporal-specialization/tree/master

## Setup
1. Clone the project from the github repository provided above.
2. Apply the patch file `temporal_specialization.patch`, located in this folder.
3. Go into the `docker-build` directory and build the Docker container using `sudo docker build -t <tagname> .`

### Setup details
This details some of the setup steps, feel free to skip if not interested, this is not needed.

The provided patch file will make the following changes:
1. Modify `createSyscallStats.py`, such that it works without the `--libdebloating` option. There was a bug where the `cfgbasepath` would not get prepended to the name of the cfg file, the patch fixes this.
2. Modify `Dockerfile`, to allow generating bitcode files. First the `ubuntu` base image version is pinned to 18.04, this is what I have used for my experiments, but other versions may work, however more recent versions of ubuntu may not have the packages that the original Dockerfile defines. Furthermore the gold linker is compiled, which is required to make bitcode files.
3. Modify `run.sh`, to perform only syscalls analysis, only using the temporal-specialization tool (no `libdebloating`).

## Usage
First run the container built in the previous section using:
```sh
sudo docker run --rm -it <tag name> /bin/bash
```

This will give you an interactive shell inside the container.
Executing `./run.sh <app name>` from `/debloating-vol/temporal-specialization-artifacts` will perform analysis on the given `<app name>` and place its results in the `stats` and `outputs` folder.

This requires `<app name>` to have a valid bitcode files in the `bitcodes` folder, which the project already provides for some versions of some applications, however for my experiments I needed to have other bitcodes as well, so as an example see below the bitcode generation for `redis` and `sqlite`.

After the bitcode is generated and the application is added to the `app.to.properties.json` file (see examples below), `run.sh` can be used to perform the analysis.

If you run into problems with bitcode generation, you can also refer to `INSTALL.md` and `COMPILE.md` in the original project files, which provide a nice and clear guide on this topic and also some more examples about compiling other projects.

### redis
Redis was rebuilt, because I wanted to have a specific version, instead of the one provided with the project.

Commands to generate bitcode for redis:
```sh
# Download specific redis version
cd /debloating-vol/temporal-specialization-artifacts/builds && mkdir redis && cd redis
wget https://download.redis.io/releases/redis-4.0.9.tar.gz
tar xvzf redis-4.0.9.tar.gz && rm redis-4.0.9.tar.gz && cd redis-4.0.9

# Seems like `hiredis` is not using the provided AR; change line 54 in deps/hiredis/Makefile to `llvm-ar`
sed -i 's/STLIB_MAKE_CMD=ar/STLIB_MAKE_CMD=llvm-ar/g' deps/hiredis/Makefile
make -j16 MALLOC=libc AR=llvm-ar CC=clang LD=/usr/local/bin/ld.gold CFLAGS="-flto -O0" LDFLAGS="-fuse-ld=gold -v -flto -Wl,-plugin-opt=save-temps" redis-server RANLIB=/bin/true

# we have `redis-server.0.5.precodegen.bc` in `src/`, this is the bitcode file we need for the analysis
cd /debloating-vol/temporal-specialization-artifacts/bitcodes
cp ../builds/redis/redis-4.0.9/src/redis-server.0.5.precodegen.bc ./my-redis-server.bc

cd /debloating-vol/temporal-specialization-artifacts/binaries
mkdir my-redis && cd my-redis
cp ../../builds/redis/redis-4.0.9/src/redis-server .
cp /lib/x86_64-linux-gnu/libm.so.6 .
cp /lib/x86_64-linux-gnu/libdl.so.2 .
cp /lib/x86_64-linux-gnu/libpthread.so.0 .
cp /lib/x86_64-linux-gnu/libc.so.6 .

# we can now use run.sh to perform the analysis
cd /debloating-vol/temporal-specialization-artifacts
./run.sh my-redis-server
```

**WORKER ENTRY:** aeMain

JSON entry for my-redis in `app.to.properties.json`:
```json
"my-redis-server": {
    "enable": "false",
    "cfg": {
            "direct" : "my-redis-server.svf.conditional.direct.calls.cfg",
            "svf" : "my-redis-server.svf.cfg",
            "svftype" : "my-redis-server.svf.type.cfg",
            "svftypefp" : "my-redis-server.svf.new.type.fp.wglobal.cfg"
           },
    "master": "main",
    "worker": "aeMain",
    "bininput": "my-redis/",
    "output": "my-redis.syscall.out"
}
```

### sqlite
Sqlite had no pre-generated bitcode, that's why it was needed to generate it here.

Commands to generate bitcode for sqlite
```sh
cd /debloating-vol/temporal-specialization-artifacts/builds && mkdir sqlite && cd sqlite
wget https://sqlite.org/2018/sqlite-autoconf-3220000.tar.gz
tar xvzf sqlite-autoconf-3220000.tar.gz && rm sqlite-autoconf-3220000.tar.gz
cd sqlite-autoconf-3220000

clang -fuse-ld=gold -flto -O0 -Wl,-plugin-opt=save-temps shell.c sqlite3.c -lpthread -ldl -lm -o sqlite3
cd /debloating-vol/temporal-specialization-artifacts/bitcodes
cp ../builds/sqlite/sqlite-autoconf-3220000/sqlite3.0.5.precodegen.bc ./sqlite.bc
cd /debloating-vol/temporal-specialization-artifacts/binaries
mkdir sqlite && cd sqlite

cp /debloating-vol/temporal-specialization-artifacts/builds/sqlite/sqlite-autoconf-3220000/sqlite3 .
cp /lib/x86_64-linux-gnu/libpthread.so.0 .
cp /lib/x86_64-linux-gnu/libdl.so.2 .
cp /lib/x86_64-linux-gnu/libm.so.6 .
cp /lib/x86_64-linux-gnu/libc.so.6 .

# we can now use run.sh to perform the analysis
cd /debloating-vol/temporal-specialization-artifacts
./run.sh sqlite
```

**WORKER ENTRY**: `shell_exec`

JSON entry for sqlite:
```json
"sqlite": {
    "enable": "false",
    "cfg": {
            "direct" : "sqlite.svf.conditional.direct.calls.cfg",
            "svf" : "sqlite.svf.cfg",
            "svftype" : "sqlite.svf.type.cfg",
            "svftypefp" : "sqlite.svf.new.type.fp.wglobal.cfg"
           },
    "master": "main",
    "worker": "shell_exec",
    "bininput": "sqlite/",
    "output": "sqlite.syscall.out"
}
```
