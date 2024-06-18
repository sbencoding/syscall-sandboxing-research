#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* thread1() {
    printf("thread 1\n");
    return NULL;
}

void* thread2() {
    printf("thread 2222\n");
    return NULL;
}

int main() {
    printf("Hi there!\n");
    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, &thread1, NULL);
    pthread_create(&tid2, NULL, &thread2, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    return 0;
}
