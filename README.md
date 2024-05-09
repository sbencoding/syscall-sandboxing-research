# Syscall tracing
This is the start of a simple tracer that aims to provide information about what syscalls were invoked during program execution.

## Project structure
* `tracer.c` - the main driving code for the tracer
* `pstree.h` - contains the process tree data structure and functions to interact with it
* `test_progs` - small programs for testing how the tracer works
* `test_progs_out` - build results of the `test_progs` programs
* `sysnr_mapping` - the mapping generator as well as generated mappings (c header files) are stored here
* `docs` - contains project related documentation about the tracer, the final paper or just general knowledge

## Implemented features
* Trace syscall invocations of a single thread of a single process
* Map x86_64 syscall numbers to names
* Colors!
* Trace child processes of the traced parent process
* Aggregate syscall statistics for a single run of the program
* Trace all threads to the traced parent process
* Killing tracer at any point kills all children and still collect and print all stats

## Missing features
* Aggregate syscall statistics for multiple runs of the program
* Devise a way to generate inputs / interact with the traced program
    - this can be just a framework for now which delegates the actual input generation/interaction to another process
    - the point of this features is to explore many program states of the traced application and hopefully explore all the system calls that it makes
