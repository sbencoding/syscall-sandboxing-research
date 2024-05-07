#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <linux/ptrace.h> // glibc doesn't necessarily expose the ptrace_syscall_info struct
#include <sys/wait.h>
#include <string.h>

#include "sysnr_map.h" // map syscall numbers to their names

void init_trace(int child_pid) {
    // TODO: PTRACE_O_TRACEEXEC
    ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK);
    ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fputs("Please provide at least the path of the executable to trace!\n", stderr);
        return 1;
    }

    printf("Going to trace: %s\n", argv[1]);
    pid_t child_pid = fork();
    if (child_pid == 0) {
        printf("Child requesting trace by the parent");
        ptrace(PTRACE_TRACEME, NULL, NULL, NULL);
        execve(argv[1], &argv[2], NULL);
    } else {
        printf("Starting up the tracer\n");
        int wstatus;
        int child_pid;
        struct ptrace_syscall_info syscall_info;

        while ((child_pid = waitpid(-1, &wstatus, 0)) != -1) {
            if (WIFSIGNALED(wstatus)) {
                // TODO: we might need to continue (forward signal) the process here for some signals
                printf("We are stopping because of a signal: %d\n", WSTOPSIG(wstatus));
            } else if (WIFEXITED(wstatus)) {
                printf("Child process exited\n");
            } else if (WIFSTOPPED(wstatus)) {
                int stop_sig = WSTOPSIG(wstatus);
                int event_code = wstatus>>8;
                switch (event_code) {
                    case (SIGTRAP | (PTRACE_EVENT_CLONE<<8)):
                    case (SIGTRAP | (PTRACE_EVENT_FORK<<8)):
                        unsigned long new_pid;
                        ptrace(PTRACE_GETEVENTMSG, child_pid, NULL, &new_pid);
                        printf("[!] Caught ptrace event! Starting tracer on %lu\n", new_pid);
                        init_trace(new_pid);
                        break;
                }
                switch (stop_sig) {
                    case SIGTRAP:
                        init_trace(child_pid);
                        break;
                    case SIGTRAP | 0x80:
                        ptrace(PTRACE_GET_SYSCALL_INFO, child_pid, sizeof(syscall_info), &syscall_info);
                        switch (syscall_info.op) {
                            case PTRACE_SYSCALL_INFO_ENTRY:
                                printf("[PID=%lu] SYS_%s [%lu] @ %#08lx", child_pid, sysnr_map[syscall_info.entry.nr], syscall_info.entry.nr, syscall_info.instruction_pointer);
                                break;
                            case PTRACE_SYSCALL_INFO_EXIT:
                                printf(" -> %ld\n", syscall_info.exit.rval);
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
                        printf("Warning: forwarding signal `%s` to process %lu\n", strsignal(stop_sig), child_pid);
                        ptrace(PTRACE_SYSCALL, child_pid, NULL, stop_sig);
                        break;
                }
            }
        }
    }
}
