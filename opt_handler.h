#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shared.h"

struct app_config {
    int is_silent;
    int is_tracee_silent;
    char* inferior_path;
    char** inferior_args;
    int phase_count;
    unsigned long phase_addr[MAX_PHASE_COUNT];
};

/*
 * -s - silence output from tracee and child procs
 * -m - minimal output, only output the total gathered syscalls across all procs. (also allows tracee output)
 * [PATH] [ARGS] - provide the path to the executable to trace and optionally its arguments.
 */
struct app_config* handle_cmdline_opts(int argc, char** argv) {
    struct app_config* conf = (struct app_config*)malloc(sizeof(struct app_config));
    int c;

    conf->phase_count = 0;
    while ((c = getopt (argc, argv, "smp:")) != -1) {
        switch (c) {
            case 'm':
                conf->is_silent = 1;
                break;
            case 's':
                conf->is_tracee_silent = 1;
                break;
            case 'p':
                if (conf->phase_count == MAX_PHASE_COUNT) {
                    fprintf(stderr, "Error: the tracer does not support more than %d phases\n", MAX_PHASE_COUNT);
                    free(conf);
                    exit(1);
                }
                conf->phase_addr[conf->phase_count++] = strtoul(optarg, NULL, 16);
                break;
            default:
                fputs("Error: invalid arguments provided!\n", stderr);
                free(conf);
                exit(1);
        }
    }

    if (optind < argc) {
        conf->inferior_path = argv[optind];
        conf->inferior_args = &argv[optind];
    } else {
        fputs("Error: missing path to executable to trace!\n", stderr);
        free(conf);
        exit(1);
    }

    return conf;
}
