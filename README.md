# System call sandboxing
This repository stores the source code, Dockerfiles, setup guides and project documents, such as the final report and poster produced to complete the [CSE3000 course](https://studiegids.tudelft.nl/a101_displayCourse.do?course_id=64466).

The project contains a simple tracer that supports both the single phase execution model and multi phase execution as well.
The tracer is based on the `ptrace` syscall and requires the test suite in order to gather syscalls used by the targeted applications.
See the sections below for the implemented and missing features, and also the repository structure.

## Project structure
* `tracer.c` - the main driving code for the tracer
* `pstree.h` - contains the process tree data structure and functions to interact with it
* `shared.h` - contains information that need to be shared among all source files
* `opt_handler.h` - contains logic to parse command line arguments into a configuration struct
* `test_progs` - small programs for testing how the tracer works
* `test_progs_out` - build results of the `test_progs` programs
* `sysnr_mapping` - the mapping generator as well as generated mappings (c header files) are stored here
* `docs` - contains project related documentation such as the final paper and the poster
* `interact` - contains the runners for the test programs that will invoke the tracer and collect stats
* `config` - Dockerfiles, patches and scripts to setup and run the analysis tools investigated during the research
* `Dockerfile` - docker build to execute the tracer in the same container environment as the other analysis tools

## Implemented features
* Trace syscall invocations of a single thread of a single process
* Map x86_64 syscall numbers to names
* Colors!
* Trace child processes of the traced parent process
* Aggregate syscall statistics for a single run of the program
* Trace all threads to the traced parent process
* Killing tracer at any point kills all children and still collect and print all stats
* Aggregate syscall statistics for multiple runs of the program
* Devise a way to generate inputs / interact with the traced program
    - the point of this feature is to explore many program states of the traced application and hopefully explore all the system calls that it makes
    - this is just a framework for now which doesn't solve the problem of input generation
* Support for multi phase tracing
    - it is possible to provide up to 10 phase transition points manually through the command line using the `-p` flag and the offset of the instruction from the start of the binary

## Missing features
* Have some automatic phase detection when forking / creating a new thread / exec'ing etc. (this is done to some extent through creating a new pstree entry for certain actions)
* Policy generation (single phase)
    - have a program that generates seccomp policy (c source code, epbf filter array etc.) for single phase execution
* Policy patching (single phase)
    - have a program that patches the test program to include a seccomp filter
* Extend policy generation to multi phase (take seccomp limitations into account, better way than seccomp?)
* Extend policy patching to multi phase (need to patch at different locations for this)

## Test programs analyzed
- [x] `ls` (v8.28)
- [x] `sqlite` (v3.22)
- [x] `redis-server` (v4.0.9)
