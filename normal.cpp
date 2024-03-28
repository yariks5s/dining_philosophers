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
        while (true)
        {
            think();
            dine();
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

    void dine() {
        // Визначення номерів лівої та правої виделок для філософа
        std::size_t leftForkIndex = name - 1;
        std::size_t rightForkIndex = name % num_philosophers;

        // Визначення порядку взяття виделок
        std::size_t firstFork = std::min(leftForkIndex, rightForkIndex);
        std::size_t secondFork = std::max(leftForkIndex, rightForkIndex);

        Fork& firstForkRef = (firstFork == leftForkIndex) ? leftFork : rightFork;
        Fork& secondForkRef = (secondFork == rightForkIndex) ? rightFork : leftFork;

        // Блокування першої виделки
        std::unique_lock<std::mutex> lockFirstFork(firstForkRef.mutex);
        while (firstForkRef.isTaken)
            firstForkRef.cv.wait(lockFirstFork);
        firstForkRef.takeFork();

        // Блокування другої виделки
        std::unique_lock<std::mutex> lockSecondFork(secondForkRef.mutex);
        while (secondForkRef.isTaken)
            secondForkRef.cv.wait(lockSecondFork);
        secondForkRef.takeFork();

        // Симуляція їжі
        print("Philosopher ", name, " is dining.\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Час на обід

        // Повернення виделок
        firstForkRef.putFork();
        lockFirstFork.unlock();
        firstForkRef.cv.notify_one();

        secondForkRef.putFork();
        lockSecondFork.unlock();
        secondForkRef.cv.notify_one();

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
