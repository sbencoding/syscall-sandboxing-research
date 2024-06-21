# chestnut
Setup and usage for the `chestnut` project (tested on commit hash 839c39d4ee91c993c92fd312ca70b7d0d7bd1b83).
Source: https://github.com/IAIK/Chestnut

## Setup
The provided `Dockerfile` takes care of the setup of `Binalyzer` and `Sourcalyzer` for the most part.
The most important note is that this requires quite some compiling work and we have some `make -j10` commands.
You can use a higher `-j16` for example to compile faster, but specifically the LLVM build ran out of memory on a 16GB machine with 18 concurrent jobs (default for the ninja build).

### Setup details
This details some of the setup steps, feel free to skip if not interested, this is not needed.

First we take care of setting up `Binalyzer`, which is just installing some python packages.
The docker build doesn't include a cool patch by Petr, which fixes some bugs with the `Binalyzer`, see more about this in the usage section below.

The next big step is building `Sourcalyzer`. It is an LLVM patch that analyzed programs while compiling them and outputs some call graph or syscall information to them.
The first step is to download, patch and compile LLVM.

Here we are using a modified version of the script provided by the authors.
The modification (`patched_loader.sh`):
1. Reduces the number of concurrent ninja build jobs (reason: out of memory issues with the default)
2. Compiles `clang` and `lld` as well, which are the frontend for the llvm project which will allow us to compile and link stuff with the autosandboxing changes.

After this is done we build `chestnut.o` in a separate script (because if this fails I wouldn't want to recompile llvm again, therefore it is not done in one docker step).
`stage2_loader.sh` modifies some of the paths to do what was intended by the authors.

Next we need to download, compile and install `musl`.
This is needed because:
1. To analyze an application all its dependencies need to be recompiled with the `-fautosandbox` option
2. This version of LLVM, can't build the version of glibc that is on the container system. And a non-gcc glibc build is troublesome in general.

Compiling and installing works surprisingly well, we just need to configure the build to use the newly built patched clang compiler.

`musl` nicely puts a wrapper over the compiler and the linker in `/usr/local/musl/bin`, which will make the compiler build stuff using the `musl` libraries instead of default system ones.
The linker wrapper is modified to make use of the patched `lld` linked instead of the system default one.

Further modifications are made the to wrapper, because the default one causes problems when running a statically compiled binary. The problem is that the default wrapper always include a dynamic linker, even if we are compiling statically.
To solve this the docker file makes a wrapper that works for static compiling, `musl-clang.static`, which should be used whenever the `-static` option is provided.

Next `libseccomp` needs to be built using the `musl` libraries.
This is needed, because when using the `-fautosandbox` option, the compiler will include the `chestnut.o` object, which depends on libseccomp. This is problematic if we are building something based on the `musl` libc, but we have an object including stuff from `glibc`.
Another trick here is symlinking some system headers in the `musl` include directory that libseccomp relies on.

`libseccomp` (tested on commit hash 9da5d174e3ef219baab020a79c789f2075ace45c) is built using the patched clang compiler. Additionally we configure it to install itself to `/usr/local/musl`, so these libraries will be used when building with `musl` and they won't conflict with the system ones.

Finally `chestnut.o` is recompiled against the `libseccomp` based on `musl` libraries.

After this the sourcealyzer is ready to use.

## Usage
This section details the usage of the 2 part of the project.

### Binalyzer
There are 2 scripts that are important here `cfg.py` and `syscalls.py`.
To properly use `syscalls.py` you are going to want to use [the patch from Petr](https://github.com/felacek/Chestnut/tree/binalyzer-fix).

`syscalls.py` just looks for any syscall instruction it can find, so probably it will have a large overestimation of what system calls are needed. This overestimation improves a bit if you run it on a statically linked binary.
It will generate the list of used system calls in the `cached_results` folder with a name like `syscalls_<path to file>`

`cfg.py` also incorporates the callgraph of the binary to determine what syscalls are used to avoid the large overapproximation.
This doesn't work on dynamically linked binaries, but it does give some decent results for statically linked ones.
It will generate the list of used system calls in the `cached_results` folder, with the name cfg.json. The `:all` array contains all syscalls used by the program, but other keys break it down on a per-function level.

### Sourcalyzer
This one is a bit tricky, but it can work.
The key idea is to compile each application you want to test, using the patched clang compiler and the `-fautosandbox` flag. You also need all dependencies of the application to be compiled with the new compiler and flag.
For most programs this is already okay as they mostly only depend on libc, which we have compiled already.

Additionally to generate the `syscalls` note, the final binary needs to be linked statically, otherwise the compiler is only going to embed the call hierarchy note.

So in general 3 compiler flags are required when compiling programs: `-static -lseccomp -fautosandbox`.
The generated final binary should have the `syscalls` note, you can hexdump its content using `readelf -x '.note.syscalls' <path to binary>`.

I put some examples below of how I compiled some test programs this way. It is not always trivial to do so.

#### coreutils
```sh
apt-get install wget
cd /tool && mkdir coreutils && cd coreutils
wget https://ftpmirror.gnu.org/coreutils/coreutils-8.28.tar.xz
unxz coreutils-8.28.tar.xz && tar xvf coreutils-8.28.tar && rm coreutils-8.28.tar
cd coreutils-8.28
export FORCE_UNSAFE_CONFIGURE=1
./configure CFLAGS='-static -lseccomp -fautosandbox' CC='/usr/local/musl/bin/musl-clang.static' --prefix='/usr/local/musl'
make -j16
```

`FORCE_UNSAFE_CONFIGURE` will allow to execute `./configure` even as root. I am using this since I didn't drop privileges in the container.

#### sqlite
```sh
apt-get install wget
cd /tool && mkdir sqlite && cd sqlite
wget https://sqlite.org/2018/sqlite-autoconf-3220000.tar.gz
tar xvzf sqlite-autoconf-3220000.tar.gz && rm sqlite-autoconf-3220000.tar.gz
cd sqlite-autoconf-3220000
/usr/local/musl/bin/musl-clang.static -static -lseccomp -fautosandbox shell.c sqlite3.c -lpthread -ldl -lm -o sqlite3
```

This is perhaps the simplest to get to work, as it doesn't even involve `make`, just directly compiling 2 source files.

#### redis
```sh
apt-get install wget
cd /tool && mkdir redis && cd redis
wget https://download.redis.io/releases/redis-4.0.9.tar.gz
tar xvzf redis-4.0.9.tar.gz && rm redis-4.0.9.tar.gz

# Patch libseccomp zmalloc and zrealloc conflict
cd /tool/seccomp/libseccomp/src
grep -RnlI 'zmalloc' | xargs sed -i 's/zmalloc/lsc_zmalloc/g'
grep -RnlI 'zrealloc' | xargs sed -i 's/zrealloc/lsc_zrealloc/g'
cd ..
make clean && make -j16 && make install

# Continue redis building
cd /tool/redis/redis-4.0.9
make -j16 MALLOC=libc CC='/usr/local/musl/bin/musl-clang.static' REDIS_LDFLAGS='-static -lseccomp -fautosandbox' CFLAGS='-lseccomp -fautosandbox' REDIS_CFLAGS='-static -lseccomp -fautosandbox' redis-server
```

This is a bit tricky to compile.
First of all both redis and libseccomp have `zmalloc` and `zrealloc`, therefore the linking fails, since we have 2 symbols with the same name, and it is not clear which one is referenced. Therefore we first recompile libseccomp with these 2 functions renamed to something else.

The second tricky thing is that redis has its own compile flags, and compile flags that affect the dependencies as well, so these need to be separately configured.
