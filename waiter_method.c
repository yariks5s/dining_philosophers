#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_PHILOSOPHERS 5

pthread_mutex_t print_mutex;
pthread_mutex_t waiter_mutex;
int forks_taken[NUM_PHILOSOPHERS] = {0};

void print(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    pthread_mutex_lock(&print_mutex);
    vprintf(format, args);
    pthread_mutex_unlock(&print_mutex);
    va_end(args);
}

bool request_forks(int leftForkIndex, int rightForkIndex)
{
    pthread_mutex_lock(&waiter_mutex);
    if (forks_taken[leftForkIndex] == 0 && forks_taken[rightForkIndex] == 0)
    {
        forks_taken[leftForkIndex] = 1;
        forks_taken[rightForkIndex] = 1;
        pthread_mutex_unlock(&waiter_mutex);
        return true;
    }
    pthread_mutex_unlock(&waiter_mutex);
    return false;
}

void release_forks(int leftForkIndex, int rightForkIndex)
{
    pthread_mutex_lock(&waiter_mutex);
    forks_taken[leftForkIndex] = 0;
    forks_taken[rightForkIndex] = 0;
    pthread_mutex_unlock(&waiter_mutex);
}

void* philosopher_action(void* arg)
{
    int philosopher = *(int*)arg;
    int leftForkIndex = philosopher;
    int rightForkIndex = (philosopher + 1) % NUM_PHILOSOPHERS;

    while (true)
    {
        print("Philosopher %d is thinking.\n", philosopher + 1);
        sleep(rand() % 3 + 1);

        print("Philosopher %d is hungry.\n", philosopher + 1);
        while (!request_forks(leftForkIndex, rightForkIndex))
            usleep(100);

        print("Philosopher %d is eating.\n", philosopher + 1);
        sleep(rand() % 3 + 1);

        release_forks(leftForkIndex, rightForkIndex);
        print("Philosopher %d finished eating and put down forks.\n", philosopher + 1);
    }
    return NULL;
}

int main()
{
    pthread_mutex_init(&print_mutex, NULL);
    pthread_mutex_init(&waiter_mutex, NULL);

    pthread_t threads[NUM_PHILOSOPHERS];
    int philosophers[NUM_PHILOSOPHERS];

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
    {
        philosophers[i] = i;
        pthread_create(&threads[i], NULL, philosopher_action, &philosophers[i]);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&print_mutex);
    pthread_mutex_destroy(&waiter_mutex);
    return 0;
}
