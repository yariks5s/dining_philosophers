#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define NUM_PHILOSOPHERS 5

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

void print(const char* format, ...)
{
    va_list args;
    pthread_mutex_lock(&print_mutex);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    pthread_mutex_unlock(&print_mutex);
}

typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cv;
    bool isTaken;
} Fork;

typedef struct
{
    size_t name;
    size_t num_philosophers;
    Fork* leftFork;
    Fork* rightFork;
} Philosopher;

void takeFork(Fork* fork)
{
    fork->isTaken = true;
    pthread_cond_signal(&fork->cv);
}

void putFork(Fork* fork)
{
    fork->isTaken = false;
    pthread_cond_signal(&fork->cv);
}

void* philosopher_action(void* arg)
{
    Philosopher* philosopher = (Philosopher*)arg;
    while (1)
    {
        print("Philosopher %zu is thinking.\n", philosopher->name);
        int r = rand() % 3000;
        usleep(r * 1000);

        size_t leftForkIndex = philosopher->name - 1;
        size_t rightForkIndex = philosopher->name % philosopher->num_philosophers;

        size_t firstForkIndex = leftForkIndex < rightForkIndex ? leftForkIndex : rightForkIndex;
        size_t secondForkIndex = leftForkIndex < rightForkIndex ? rightForkIndex : leftForkIndex;

        Fork* firstFork = (firstForkIndex == leftForkIndex) ? philosopher->leftFork : philosopher->rightFork;
        Fork* secondFork = (secondForkIndex == rightForkIndex) ? philosopher->rightFork : philosopher->leftFork;

        pthread_mutex_lock(&firstFork->mutex);
        while (firstFork->isTaken)
            pthread_cond_wait(&firstFork->cv, &firstFork->mutex);

        takeFork(firstFork);

        pthread_mutex_lock(&secondFork->mutex);
        while (secondFork->isTaken)
            pthread_cond_wait(&secondFork->cv, &secondFork->mutex);

        takeFork(secondFork);

        // Dining
        print("Philosopher %zu is dining. So he took fork #%zu and #%zu\n", philosopher->name, firstForkIndex, secondForkIndex);
        r = rand() % 3000;
        usleep(r * 1000);

        putFork(firstFork);
        pthread_mutex_unlock(&firstFork->mutex);
        pthread_cond_signal(&firstFork->cv);

        putFork(secondFork);
        pthread_mutex_unlock(&secondFork->mutex);
        pthread_cond_signal(&secondFork->cv);

        print("Philosopher %zu finished dining.\n", philosopher->name);
    }

    return NULL;
}

int main()
{
    srand(time(NULL));

    Fork forks[NUM_PHILOSOPHERS];
    Philosopher philosophers[NUM_PHILOSOPHERS];
    pthread_t threads[NUM_PHILOSOPHERS];

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
    {
        forks[i].isTaken = false;
        pthread_mutex_init(&forks[i].mutex, NULL);
        pthread_cond_init(&forks[i].cv, NULL);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
    {
        philosophers[i].name = i + 1;
        philosophers[i].leftFork = &forks[i];
        philosophers[i].rightFork = &forks[(i + 1) % NUM_PHILOSOPHERS];
        philosophers[i].num_philosophers = NUM_PHILOSOPHERS;
        pthread_create(&threads[i], NULL, philosopher_action, &philosophers[i]);
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
        pthread_join(threads[i], NULL);

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
    {
        pthread_mutex_destroy(&forks[i].mutex);
        pthread_cond_destroy(&forks[i].cv);
    }

    return 0;
}
