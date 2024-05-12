#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct app_config {
    int is_silent;
    char* inferior_path;
    char** inferior_args;
};

/*
 * -m - minimal output, only output the total gathered syscalls across all procs. (also allows tracee output)
 * [PATH] [ARGS] - provide the path to the executable to trace and optionally its arguments.
 */
struct app_config* handle_cmdline_opts(int argc, char** argv) {
    struct app_config* conf = (struct app_config*)malloc(sizeof(struct app_config));
    int c;

    while ((c = getopt (argc, argv, "m")) != -1) {
        switch (c) {
            case 'm':
                conf->is_silent = 1;
                break;
            default:
                fputs("Error: invalid arguments provided!\n", stderr);
                free(conf);
                exit(1);
        }
    }

    if (optind < argc) {
        conf->inferior_path = argv[optind];
        conf->inferior_args = &argv[optind+1];
    } else {
        fputs("Error: missing path to executable to trace!\n", stderr);
        free(conf);
        exit(1);
    }

    return conf;
}
