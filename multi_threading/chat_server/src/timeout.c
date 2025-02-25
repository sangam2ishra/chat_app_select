#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include "timeout.h"
#include <time.h>

static void *timeout_checker(void *arg) {
    while (1) {
         sleep(60); // Check every 60 seconds.
         // (Optional) Add inactivity timeout logic here.
    }
    return NULL;
}

void start_timeout_checker() {
    pthread_t tid;
    if (pthread_create(&tid, NULL, timeout_checker, NULL) != 0) {
         perror("Could not create timeout thread");
    } else {
         pthread_detach(tid);
    }
}
