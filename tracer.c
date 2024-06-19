#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h> // glibc doesn't necessarily expose the ptrace_syscall_info struct
#include <sys/wait.h>
#include <sys/user.h> // for user_regs_struct
#include <string.h>
#include <limits.h> // for PATH_MAX
#include <signal.h>
#include <fcntl.h>

#include "shared.h" // data shared between multiple source files
#include "sysnr_map.h" // map syscall numbers to their names
#include "pstree.h" // Process tree info storage
#include "opt_handler.h" // Helper file to handle command line arguments

#define VERBOSE_TRACE
#define silent_printf(...) if (conf->is_silent == 0) printf(__VA_ARGS__)

pid_t root_child_pid;

unsigned long get_proc_base_addr(unsigned long pid) {
    char cur_proc_maps[PATH_MAX];
    snprintf(cur_proc_maps, PATH_MAX, "/proc/%lu/maps", pid);
    FILE* map_file = fopen(cur_proc_maps, "r");
    char base_addr_buf[20] = {};
    fread(base_addr_buf, 20, 1, map_file);
    base_addr_buf[19] = '\0';

    unsigned long base_address = 0;
    sscanf(base_addr_buf, "%llx-", &base_address);

    fclose(map_file);
    return base_address;
}

void mk_banner(const char* banner_text) {
    printf("\n\033[1;35m====================%s====================\033[0m\n\n", banner_text);
}

unsigned long breakpoint_original_data[MAX_PHASE_COUNT];
int is_root = 1;

void init_trace(int child_pid, struct app_config* conf) {
    if (is_root == 1 && conf->phase_count != 0) {
        is_root = 0;
        if (conf->is_silent == 0) mk_banner("Multi-phase setup");
        unsigned long base_addr = get_proc_base_addr(child_pid);
        silent_printf("Process base address is @ \033[1;32m0x%lx\033[0m\n", base_addr);
        for (int i = 0; i < conf->phase_count; ++i) {
            unsigned long phase_addr = base_addr + conf->phase_addr[i];
            conf->phase_addr[i] = phase_addr;
            silent_printf("Setting breakpoint at address \033[1;36m0x%lx\033[0m\n", phase_addr);
            breakpoint_original_data[i] = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)phase_addr, NULL);
            ptrace(PTRACE_POKETEXT, child_pid, (void*)phase_addr, 0xcc);
        }

        silent_printf("\n");
    }
    // TODO: vfork
    ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEEXEC);
    ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
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
    if (node->current_phase != 0) {
        puts("\n\033[1;36mInit phase\033[0m\n");
    }
    print_flat_syscall_list(node->syscall_usage);

    for (int i = 0; i < MAX_PHASE_COUNT; ++i) {
        if (node->extra_phase_syscall_usage[i] == NULL) continue;
        printf("\n\033[1;36mPhase %d\033[0m\n", i + 1);
        print_flat_syscall_list(node->extra_phase_syscall_usage[i]);
    }
}

void ps_callback_syscall_stats(struct pstree* node) {
    printf("\n\033[1;32mPID=%lu\033[0m - %s\n", node->pid, node->exe_path);
    if (node->current_phase != 0) {
        puts("\n\033[1;36mInit phase\033[0m\n");
    }
    print_syscall_stats(node->syscall_usage);
    for (int i = 0; i < MAX_PHASE_COUNT; ++i) {
        if (node->extra_phase_syscall_usage[i] == NULL) continue;
        printf("\n\033[1;36mPhase %d\033[0m\n", i + 1);
        print_syscall_stats(node->extra_phase_syscall_usage[i]);
    }
}

void phase_tree_visitor(struct pstree* node, int cur_phase, const int max_phase, int usage_arr[][N_SYSCALLS]) {
    for (int i = 0; i < N_SYSCALLS; ++i) {
        usage_arr[cur_phase][i] += node->syscall_usage[i];
    }

    for (int i = 0; i < max_phase; ++i) {
        if (node->extra_phase_syscall_usage[i] == NULL) continue;
        for (int j = 0; j < N_SYSCALLS; ++j) {
            usage_arr[i + 1][j] += node->extra_phase_syscall_usage[i][j];
        }
    }

    if (node->exec != NULL) phase_tree_visitor(node->exec, node->current_phase, max_phase, usage_arr);
    if (node->sibling != NULL) phase_tree_visitor(node->sibling, node->sibling->current_phase, max_phase, usage_arr);
    if (node->child != NULL) phase_tree_visitor(node->child, node->child->current_phase, max_phase, usage_arr);
}

void print_flat_phase_list(struct pstree* root, int phase_count) {
    // +1 because we have an initial default phase
    int phase_syscall_usage[phase_count + 1][N_SYSCALLS] = {};
    phase_tree_visitor(root, 0, phase_count, phase_syscall_usage);

    for (int i = 0; i <= phase_count; ++i) {
        printf("%d:", i);
        print_flat_syscall_list(phase_syscall_usage[i]);
    }
}

int syscall_usage[N_SYSCALLS] = {};
char cur_proc_exe_link[PATH_MAX];

char* get_proc_path(unsigned long pid) {
    snprintf(cur_proc_exe_link, PATH_MAX, "/proc/%lu/exe", pid);
    char* exe_path = (char*) malloc(PATH_MAX * sizeof(char));
    readlink(cur_proc_exe_link, exe_path, PATH_MAX * sizeof(char));
    return exe_path;
}

int address_to_phase_index(struct app_config* conf, unsigned long addr) {
    for (int i = 0; i < conf->phase_count; ++i) {
        if (conf->phase_addr[i] == addr) {
            return i;
        }
    }
    return -1;
}

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
                        ps_child->current_phase = ps_parent->current_phase; // Child inherits the phase of the parent
                        pstree_insert_child(ps_parent, ps_child);

                        init_trace(new_pid, conf);
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
                        struct user_regs_struct regs;
                        ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
                        regs.rip--;

                        int phase_idx = address_to_phase_index(conf, regs.rip);
                        silent_printf("SIGTRAP! @ 0x%lx with phase idx %d\n", regs.rip, phase_idx);
                        if (phase_idx != -1) {
                            struct pstree* ps_node = pstree_find(ps_root, child_pid);
                            ps_node->current_phase = phase_idx + 1; // +1, because phase 0 is before we hit any phases
                            silent_printf("TMP BREAKPOINT! @ 0x%lx\n", regs.rip);
                            ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
                            ptrace(PTRACE_POKETEXT, child_pid, (void*) regs.rip, breakpoint_original_data[phase_idx]);
                            ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
                        } else {
                            init_trace(child_pid, conf);
                        }
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
                                pstree_register_syscall(caller, syscall_info.entry.nr);
#else
                                silent_printf("[PID=%lu] \033[1;32mSYS_%s\033[0m [%lu] @ %#08lx", child_pid, sysnr_map[syscall_info.entry.nr], syscall_info.entry.nr, syscall_info.instruction_pointer);
#endif
                                break;
                            case PTRACE_SYSCALL_INFO_EXIT:
#ifdef VERBOSE_TRACE
                                // FIXME: execve doesn't get printed here!
                                caller = pstree_find(ps_root, child_pid);
                                silent_printf("[PID=%lu:%d] \033[1;32mSYS_%s\033[0m [%lu] @ %#08lx -> %ld\n", child_pid, caller->current_phase, sysnr_map[caller->syscall.syscall_nr], caller->syscall.syscall_nr, caller->syscall.ip, syscall_info.exit.rval);
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

        if (conf->phase_count != 0) {
            if (conf->is_silent == 0) {
                mk_banner("Per phase syscall flat list");
            }

            print_flat_phase_list(ps_root, conf->phase_count);
        }

        pstree_free(ps_root);
        free(conf);
    }
}
