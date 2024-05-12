#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h> // glibc doesn't necessarily expose the ptrace_syscall_info struct
#include <sys/wait.h>
#include <string.h>
#include <limits.h> // for PATH_MAX
#include <signal.h>
#include <fcntl.h>

#include "sysnr_map.h" // map syscall numbers to their names
#include "pstree.h" // Process tree info storage
#include "opt_handler.h" // Helper file to handle command line arguments

#define VERBOSE_TRACE
#define silent_printf(...) if (conf->is_silent == 0) printf(__VA_ARGS__)

void init_trace(int child_pid) {
    // TODO: vfork
    ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEEXEC);
    ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
}

void mk_banner(const char* banner_text) {
    printf("\n\033[1;35m====================%s====================\033[0m\n\n", banner_text);
}

void print_syscall_stats(int* usage_array) {
    for (int i = 0; i < N_SYSCALLS; ++i) {
        if (usage_array[i] != 0) {
            printf("%s - %d\n", sysnr_map[i], usage_array[i]);
        }
    }
}

void print_flat_syscall_list(int* usage_array) {
    for (int i = 0; i < N_SYSCALLS; ++i) {
        if (usage_array[i] != 0) {
            printf("%s,", sysnr_map[i]);
        }
    }
    putchar('\n');
}

void ps_callback_syscall_list(struct pstree* node) {
    printf("\n\033[1;32mPID=%lu\033[0m - %s\n", node->pid, node->exe_path);
    print_flat_syscall_list(node->syscall_usage);
}

void ps_callback_syscall_stats(struct pstree* node) {
    printf("\n\033[1;32mPID=%lu\033[0m - %s\n", node->pid, node->exe_path);
    print_syscall_stats(node->syscall_usage);
}

int syscall_usage[N_SYSCALLS] = {};
char cur_proc_exe_link[PATH_MAX];

char* get_proc_path(unsigned long pid) {
    snprintf(cur_proc_exe_link, PATH_MAX, "/proc/%lu/exe", pid);
    char* exe_path = (char*) malloc(PATH_MAX * sizeof(char));
    readlink(cur_proc_exe_link, exe_path, PATH_MAX * sizeof(char));
    return exe_path;
}

pid_t root_child_pid;

void sigint_handler(int dummy) {
    puts("\n\033[1;33m[!]\033[0m SIGINT on tracer\n");
    kill(root_child_pid, SIGINT);
}

int main(int argc, char** argv) {
    struct app_config* conf = handle_cmdline_opts(argc, argv);

    signal(SIGINT, sigint_handler);

    silent_printf("Going to trace: %s\n", conf->inferior_path);
    root_child_pid = fork();
    if (root_child_pid == 0) {
        silent_printf("Child requesting trace by the parent\n");
        if (conf->is_tracee_silent) {
            int black_hole = open("/dev/null", O_WRONLY);
            dup2(black_hole, STDOUT_FILENO);
        }
        ptrace(PTRACE_TRACEME, NULL, NULL, NULL);
        execve(conf->inferior_path, conf->inferior_args, NULL);
    } else {
        if (conf->is_silent == 0) mk_banner("Starting up the tracer");
        int wstatus;
        int child_pid;
        struct ptrace_syscall_info syscall_info;
        struct pstree* ps_root = pstree_mknode(root_child_pid, NULL);

        while ((child_pid = waitpid(-1, &wstatus, 0)) != -1) {
            if (ps_root->exe_path == NULL) {
                ps_root->exe_path = get_proc_path(ps_root->pid);
            }

            if (WIFSIGNALED(wstatus)) {
                silent_printf("\033[1;33m[!]\033[0m Child process %lu is stopping because of signal: %s\n", child_pid, strsignal(WTERMSIG(wstatus)));
            } else if (WIFEXITED(wstatus)) {
                silent_printf("\033[1;33m[!]\033[0m Child process %lu exited with status %d\n", child_pid, WEXITSTATUS(wstatus));
            } else if (WIFSTOPPED(wstatus)) {
                int stop_sig = WSTOPSIG(wstatus);
                int event_code = wstatus>>8;
                struct pstree* ps_parent;
                unsigned long new_pid;
                switch (event_code) {
                    case (SIGTRAP | (PTRACE_EVENT_CLONE<<8)):
                    case (SIGTRAP | (PTRACE_EVENT_FORK<<8)):
                        ptrace(PTRACE_GETEVENTMSG, child_pid, NULL, &new_pid);
                        silent_printf("\033[1;33m[!]\033[0m Caught ptrace event! Starting tracer on %lu\n", new_pid);
                        ps_parent = pstree_find(ps_root, child_pid);
                        struct pstree* ps_child = pstree_mknode(new_pid, strdup(ps_parent->exe_path));
                        pstree_insert_child(ps_parent, ps_child);

                        init_trace(new_pid);
                        break;
                    case (SIGTRAP | (PTRACE_EVENT_EXEC<<8)):
                        unsigned long new_pid;
                        ptrace(PTRACE_GETEVENTMSG, child_pid, NULL, &new_pid);
                        // TODO: what if the exec fails? - I wouldn't want a new pstree entry in this case.
                        silent_printf("\033[1;33m[!]\033[0m Caught ptrace EXEC event! Continuing tracer on %lu\n", new_pid);
                        struct pstree* ps_exec = pstree_mknode(new_pid, get_proc_path(new_pid));
                        ps_parent = pstree_find(ps_root, child_pid);
                        pstree_insert_exec(ps_parent, ps_exec);
                        break;
                }
                switch (stop_sig) {
                    case SIGTRAP:
                        init_trace(child_pid);
                        break;
                    case SIGTRAP | 0x80:
                        ptrace(PTRACE_GET_SYSCALL_INFO, child_pid, sizeof(syscall_info), &syscall_info);
#ifdef VERBOSE_TRACE
                        struct pstree* caller;
#endif
                        switch (syscall_info.op) {
                            case PTRACE_SYSCALL_INFO_ENTRY:
                            syscall_usage[syscall_info.entry.nr]++;
#ifdef VERBOSE_TRACE
                                caller = pstree_find(ps_root, child_pid);
                                caller->syscall.ip = syscall_info.instruction_pointer;
                                caller->syscall.syscall_nr = syscall_info.entry.nr;
                                caller->syscall_usage[syscall_info.entry.nr]++;
#else
                                silent_printf("[PID=%lu] \033[1;32mSYS_%s\033[0m [%lu] @ %#08lx", child_pid, sysnr_map[syscall_info.entry.nr], syscall_info.entry.nr, syscall_info.instruction_pointer);
#endif
                                break;
                            case PTRACE_SYSCALL_INFO_EXIT:
#ifdef VERBOSE_TRACE
                                caller = pstree_find(ps_root, child_pid);
                                silent_printf("[PID=%lu] \033[1;32mSYS_%s\033[0m [%lu] @ %#08lx -> %ld\n", child_pid, sysnr_map[caller->syscall.syscall_nr], caller->syscall.syscall_nr, caller->syscall.ip, syscall_info.exit.rval);
#else
                                silent_printf(" -> %ld\n", syscall_info.exit.rval);
#endif
                                break;
                            case PTRACE_SYSCALL_INFO_SECCOMP:
                                puts("This syscall contains seccomp info, not yet handled by the tracer");
                                break;
                            case PTRACE_SYSCALL_INFO_NONE:
                                puts("This syscall contains no data");
                                break;
                        }
                        ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
                        break;
                    default:
                        silent_printf("\033[1;33mWarning:\033[0m forwarding signal `%s` to process %lu\n", strsignal(stop_sig), child_pid);
                        ptrace(PTRACE_SYSCALL, child_pid, NULL, stop_sig);
                        break;
                }
            }
        }

        if (conf->is_silent == 0) {
            mk_banner("Recorded PS Tree");
            pstree_print(ps_root, 0);

            mk_banner("Per process syscall stats");
            pstree_foreach(ps_root, &ps_callback_syscall_stats);

            mk_banner("Per process syscall flat list");
            pstree_foreach(ps_root, &ps_callback_syscall_list);

            mk_banner("Recorded total syscalls");
            print_syscall_stats(syscall_usage);

            mk_banner("Flat total syscall list");
        }

        print_flat_syscall_list(syscall_usage);

        pstree_free(ps_root);
        free(conf);
    }
}
