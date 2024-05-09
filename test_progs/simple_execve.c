#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Hi there!\n");
    char *args[] = {"/bin/ls", "-la", NULL};
    execve("/bin/ls", args, NULL);
    printf("Exec failed\n");
    return 0;
}
