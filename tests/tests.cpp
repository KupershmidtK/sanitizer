#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <sanitizer.h>
#include <pthread.h>
#include<unistd.h>
#include <ctime>
#include <cstdlib>

#define N 5
pthread_mutexattr_t attr;
static pthread_mutex_t forks[N];

void eat() {
    std::srand(std::time(nullptr));
    int random_value = std::rand() % 100;
    usleep(random_value * 1000);
}

void* wrong_thread_func(void* arg) {
    int id = *(int*)arg;

    int left = (id - 1 + N) % N;
    int right = (id + 1) % N;

    for (size_t i = 0; i < 3; i++) {
        pthread_mutex_lock(&forks[left]);
        pthread_mutex_lock(&forks[right]);

        eat();
        
        pthread_mutex_unlock(&forks[right]);
        pthread_mutex_unlock(&forks[left]);
    }
    
    return nullptr;
}

void* correct_thread_func(void* arg) {
    int id = *(int*)arg;

    int left, right;

    if (id == N - 1) {
        right = (id - 1 + N) % N;
        left = (id + 1) % N; 
    } else {
        left = (id - 1 + N) % N;
        right = (id + 1) % N;   
    }

    for (size_t i = 0; i < 3; i++) {
        pthread_mutex_lock(&forks[left]);
        pthread_mutex_lock(&forks[right]);

        eat();
        
        pthread_mutex_unlock(&forks[right]);
        pthread_mutex_unlock(&forks[left]);
    }
    
    return nullptr;
}

TEST_CASE("No deadlock philosophers") { 
    pthread_t philosophers[N];
    int ids[N] = {0, 1, 2, 3, 4};
    for (int i = 0; i < N; i++) {
       pthread_create(&philosophers[i], NULL, correct_thread_func, &ids[i]); 
    }
    
    for (int i = 0; i < N; i++) {
       pthread_join(philosophers[i], NULL); 
    } 

    CHECK(mutexGraph.get_deadlock_count() == 0); 
}

TEST_CASE("Deadlock philosophers") { 
    pthread_t philosophers[N];
    int ids[N] = {0, 1, 2, 3, 4};
    for (int i = 0; i < N; i++) {
       pthread_create(&philosophers[i], NULL, wrong_thread_func, &ids[i]); 
    }
    
    for (int i = 0; i < N; i++) {
       pthread_join(philosophers[i], NULL); 
    } 

    CHECK(mutexGraph.get_deadlock_count() != 0); 
}