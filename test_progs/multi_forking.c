#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Hi there!\n");
    pid_t cpid = fork();
    if (cpid == 0) {
        pid_t ccpid = fork();
        if (ccpid == 0) {
            printf("Hello from child!\n");
        } else {
            printf("Hello from grandchild!\n");
        }
    } else {
        printf("Hello from parent!\n");
        waitpid(cpid, NULL, 0);
    }
    return 0;
}
