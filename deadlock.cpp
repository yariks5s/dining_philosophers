#pragma pack(push, 1) 
#pragma pack(pop)

#include <atomic>
#include <cstddef>
#include <stdlib.h> 
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <condition_variable>


std::ostream&
print_one(std::ostream& os)
{
    return os;
}

template <class A0, class ...Args>
std::ostream&
print_one(std::ostream& os, const A0& a0, const Args& ...args)
{
    os << a0;
    return print_one(os, args...);
}

template <class ...Args>
std::ostream&
print(std::ostream& os, const Args& ...args)
{
    return print_one(os, args...);
}

std::mutex&
get_cout_mutex()
{
    static std::mutex m;
    return m;
}

template <class ...Args>
std::ostream&
print(const Args& ...args)
{
    std::lock_guard<std::mutex> _(get_cout_mutex());
    return print(std::cout, args...);
}

struct Fork
{
    std::mutex mutex;
    std::condition_variable cv;
    bool isTaken = false;

    void takeFork()
    {
        isTaken = true;
        cv.notify_one();
    }

    void putFork()
    {
        isTaken = false;
        cv.notify_one();
    }
};

class Philosopher
{
public:
    std::size_t name;
    std::size_t name2;
    std::size_t num_philosophers;
    Fork& leftFork;
    Fork& rightFork;
    bool hungry = false;

    Philosopher(std::size_t name, Fork& leftFork, Fork& rightFork, size_t num_philos)
        : name(std::move(name)), leftFork(leftFork), rightFork(rightFork), num_philosophers(num_philos)
    {
        name2 = name == 5 ? 1 : name + 1;
    }

    ~Philosopher() = default;

    void action()
    {
        size_t i = 0;
        while (i < 100)
        {
            think();
            dine();
            i++;
        }
    }

    void think()
    {
        print("Philosopher ", name, " is thinking.\n\n");
        // srand (time(NULL));
        // auto rand_n = rand() % 2 + 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // thinking time
        dine();
    }

    void dine()
    {
        print("Philosopher ", name, " is trying to dine.\n");

        std::unique_lock<std::mutex> lf(leftFork.mutex);
        bool tookLeftFork = leftFork.cv.wait_for(lf, std::chrono::milliseconds(1000), [this] { return !leftFork.isTaken; });
        if (!tookLeftFork)
        {
            std::cout << "Philosopher " << name << " couldn't take the left fork and starts thinking again.\n\n";
            return; // Could not take the left fork. Return to thinking
        }

        leftFork.takeFork();

        std::unique_lock<std::mutex> rf(rightFork.mutex);
        bool tookRightFork = rightFork.cv.wait_for(rf, std::chrono::milliseconds(1000), [this] { return !rightFork.isTaken; });
        if (!tookRightFork)
        {
            std::cout << "Philosopher " << name << " couldn't take the right fork and starts thinking again.\n\n";
            leftFork.putFork();
            lf.unlock();
            leftFork.cv.notify_one();
            return; // Could not take the right fork. Return to thinking
        }

        rightFork.takeFork();

        print("Philosopher ", name, " is dining.\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Return forks back
        rightFork.putFork();
        leftFork.putFork();
        rf.unlock();
        lf.unlock();
        rightFork.cv.notify_one();
        leftFork.cv.notify_one();

        print("Philosopher ", name, " finished dining.\n");
    }

};


int main()
{
    srand (time(NULL));
    const std::size_t num_philosophers = 5;
    std::vector<Fork> forks(num_philosophers);
    std::vector<Philosopher> philosophers;

    for (int i = 0; i < num_philosophers; ++i)
        philosophers.emplace_back((i + 1), forks[i], forks[(i + 1) % num_philosophers], num_philosophers);

    std::vector<std::thread> threads;
    for (auto& philosopher : philosophers)
        threads.emplace_back(&Philosopher::action, &philosopher);

    for (auto& thread : threads)
        thread.join();
}
