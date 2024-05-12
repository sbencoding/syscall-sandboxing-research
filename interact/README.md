# Interactor
This directory contains python scripts that are used to run the test programs through the tracer and gather the results from the tracer.

This is used because for a given test program that is more complicated we would want to *"interact"* with it either through different command line arguments or stdio to explore different execution paths that might call different syscalls.

* `anl_simple_progs.py` - contains a single case for the simple test programs created to test various features of the tracer.
* `runall.py` - runs all the cases for all the defined test program runners
* `common.py` - contains some shared logic that is used to run and interact with programs and collect the results of the tracer.
