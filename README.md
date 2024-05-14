# Syscall tracing
This is the start of a simple tracer that aims to provide information about what syscalls were invoked during program execution.

## Project structure
* `tracer.c` - the main driving code for the tracer
* `pstree.h` - contains the process tree data structure and functions to interact with it
* `opt_handler.h` - contains logic to parse command line arguments into a configuration struct
* `test_progs` - small programs for testing how the tracer works
* `test_progs_out` - build results of the `test_progs` programs
* `sysnr_mapping` - the mapping generator as well as generated mappings (c header files) are stored here
* `docs` - contains project related documentation about the tracer, the final paper or just general knowledge
* `interact` - contains the runners for the test programs that will invoke the tracer and collect stats

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
    - the point of this features is to explore many program states of the traced application and hopefully explore all the system calls that it makes
    - this is just a framework for now which doesn't solve the problem of input generation

## Missing features
* More support for multi phase tracing
    - support a thread/process indicating phases (through signals for example)
    - have some automatic phase detection when forking / creating a new thread / execing etc. (this is done to some extent through creating a new pstree entry for certain actions)
    - make pstree entry support multiple phases (could reuse/repurpose the current `exec` field)
    - record thread/process entry points relative to the base address
* Policy generation (single phase)
    - have a program that generates seccomp policy (c source code, epbf filter array etc.) for single phase execution
* Policy patching (single phase)
    - have a program that patches the test program to include a seccomp filter
* Extend policy generation to multi phase (take seccomp limitations into account, better way than seccomp?)
* Extend policy patching to multi phase (need to patch at different locations for this)

## Test programs to analyze in single phase mode
For the following programs we should implement the runners and potentially extend the tracer to support them.
Focus for now is on single phase execution, so one big syscall list per program.
- [x] `ls`
- [x] `sqlite`
- [ ] `redis`
