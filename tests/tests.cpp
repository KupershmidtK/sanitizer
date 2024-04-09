#include <pthread.h>
#include<unistd.h>

pthread_mutexattr_t attr;
static pthread_mutex_t m1, m2, m3;


void* thread_func(void* arg) {
    (void)arg;

    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m3);
    sleep(1);
    pthread_mutex_unlock(&m3);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);

    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m3);
    sleep(1);
    pthread_mutex_unlock(&m3);
    pthread_mutex_unlock(&m1);

    return nullptr;
}

void* thread_func2(void* arg) {
    (void)arg;

    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m3);
    pthread_mutex_lock(&m1);
    sleep(1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m3);
    pthread_mutex_unlock(&m2);

    return nullptr;
}

int main() {
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&m1, &attr);

    pthread_t th1, th2;
    pthread_create(&th1, NULL, thread_func, NULL);
    pthread_join(th1, NULL);

    pthread_create(&th2, NULL, thread_func2, NULL);
    pthread_join(th2, NULL);

    return 0;
}