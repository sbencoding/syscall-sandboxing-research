TEST_PROG_SOURCES := $(wildcard test_progs/*.c)
TEST_PROGS := $(TEST_PROG_SOURCES:test_progs/%.c=test_progs_out/%)

.PHONY: all tracer test_progs

all: tracer test_progs

tracer: tracer.c
	cc -Isysnr_mapping tracer.c -o tracer

test_progs: $(TEST_PROGS) ; $(info $$TEST_PROGS is [${TEST_PROGS}])

$(TEST_PROGS): test_progs_out/% : test_progs/%.c
	cc $< -o $@
