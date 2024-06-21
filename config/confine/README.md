# Confine
Setup and run `Confine` (tested on commit hash a75dad3ad336e6258a42cd1ce4af061eea68ed76).
Source: https://github.com/shamedgh/confine

## Setup
```sh
git clone https://github.com/shamedgh/confine.git
cd confine
mkdir results output
# copy `confine.patch` and `test.json` here
git apply confine.patch

# build docker container that you wish to analyze using:
# sudo docker build -t confine_target .
# ensure `test.json` contains the name of your newly built container

# put the path of the binary(ies) you are interested in in `interest_bins.txt`
# if you have multiple binaries put one per line

# install sysdig or execsnoop

sudo python confine.py -l libc-callgraphs/glibc.callgraph -m libc-callgraphs/musllibc.callgraph -i test.json -o output/ -p default.seccomp.json -r results/ -g go.syscalls/ -d
```

### The patch
I decided to patch the projct because it analyzes the whole container and thus includes many other binaries besides the one I was targeting.
For a better comparison with the other tools which target a single binary I thought this was not desired.
As a result all path of binaries you are interested in should go into the `interest_bins.txt` file in the root of the project (it won't exist, you have to create it). No need to specify libraries as well, just the path to the binary you wish to analyze, for example I specified only `/usr/bin/sqlite3` when analyzing sqlite.
If you are invoking multiple binaries for a single program put one path per line

I also added a timer to the binary analysis phase, which only includes that analysis time of the extracted binaries (and not the extraction, or the checking after applying the new seccomp filter).
The timer value will be printed between the `****` lines in nanoseconds.

### test.json
Specifies which containers to test using `Confine`, this simple one should be fine for us.
Make sure your container has the same name as the one specified in this file.

### Building the container
Pretty much any container can be built, an example I have used for `sqlite` is provided in `Dockerfile`.
I guess it should end with a `CMD` statement that starts the binary you wish to analyze.

The program can't analyze short lived containers (read: containers which run for less than around a minute), so if you wish to test something short running consider using (for example):
```Dockerfile
CMD /bin/ls && /bin/sleep 180
```
*note:* running `sleep` is fine, since it is not in `interest_bins.txt` it won't get picked up for analysis.

### interest_bins.txt
As explained in the patch section put the path of any binaries you are analyzing within the container here.
If you have multiple binaries put one per line.
The patched tool will ignore any other binaries it sees and only analyze the ones provided here.

### Dependencies
The tool by default uses `sysdig` to catch any `execve` calls to see what binaries were executed.
On arch this can be found in the `sysdig` and `sysdig-dkms` packages. You may need to `sudo modprobe sysdig_probe` after installing.
For me `sysdig` crashed as soon as I ran any docker containers therefore I had to use an alternative.
If `sysdig` works for you it is better to stick to it.

The tool also supports `execsnoop`. On arch this is in the `bcc-tools` package and the binary we need is in `/usr/share/bcc/tools/execsnoop`.
If the binary on your system is in a different location you will need to update it in `python-utils/execsnoop.py` on line 40.
To make the tool use `execsnoop` instead of `sysdig` you will want to specify `--monitoringtool execsnoop` at the end of the last python command in the setup guide.

## Usage
The main command (as written in the setup guide):
```sh
sudo python confine.py -l libc-callgraphs/glibc.callgraph -m libc-callgraphs/musllibc.callgraph -i test.json -o output/ -p default.seccomp.json -r results/ -g go.syscalls/ -d
```

This will run twice and each run will take at least 60 seconds.
The tool will report that the container can't run with the filtered syscalls, this is okay, since with the patch we are only considering some of the binaries of the containers, so it is possible that others make system calls that get blocked this way.

The first run already reports timing results and the set of blocked system calls.
The system calls end up in the `results` folder in a file `<container_name>.seccomp.json`.
You can find the names of all **blocked** system calls in the file.
