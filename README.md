# Syscall tracing
This is the start of a simple tracer that aims to provide information about what syscalls were invoked during program execution.

## Project structure
* `tracer.c` - the tracer, so far it is a single file
* `test_progs` - small programs for testing how the tracer works
* `test_progs_out` - build results of the `test_progs` programs
* `sysnr_mapping` - the mapping generator as well as generated mappings (c header files) are stored here

## Implemented features
* Trace syscall invocations of a single thread of a single process
* Map x86_64 syscall numbers to names

## Missing features
* Colors!
* Trace child processes of the traced parent process
* Trace all threads to the traced parent process
* Aggregate syscall statistics for a single run of the program
* Aggregate syscall statistics for multiple runs of the program
* Devise a way to generate inputs / interact with the traced program
    - this can be just a framework for now which delegates the actual input generation/interaction to another process
    - the point of this features is to explore many program states of the traced application and hopefully explore all the system calls that it makes
