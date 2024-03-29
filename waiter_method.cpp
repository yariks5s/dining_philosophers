#include <atomic>
#include <cstddef>
#include <stdlib.h> 
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
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

struct Waiter
{
    Waiter (std::vector<Fork>& forks_) : forks(forks_) {}
    bool takeIfForkAvailable(size_t idl, size_t idr, bool second_fork = false)
    {
        if (howMuchTaken() == 4 and !second_fork)
            return false;
        std::lock_guard lk(forks[idl].mutex);
        if (!forks[idl].isTaken)
        {
            forks[idl].takeFork();
            return true;
        }
        return false;
    }

    void putFork(size_t id)
    {
        std::lock_guard lk(forks[id].mutex);
        forks[id].putFork();
    }

private:
    std::vector<Fork>& forks;

    size_t howMuchTaken()
    {
        size_t counter = 0;
        for (auto& i : forks)
        {
            if (i.isTaken)
                counter++;
        }
        return counter;
    }
};


class Philosopher
{
public:
    std::size_t name;
    std::size_t num_philosophers;
    size_t leftFork;
    size_t rightFork;
    bool hungry = false;
    Waiter wt;

    Philosopher(std::size_t name, size_t leftFork, size_t rightFork, size_t num_philos, Waiter wt_)
        : name(std::move(name)), leftFork(leftFork), rightFork(rightFork), num_philosophers(num_philos), wt(wt_)
    {}

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

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    void dine() {
        print("Philosopher ", name, " is trying to dine.\n\n");
        std::size_t leftForkIndex = name - 1;
        std::size_t rightForkIndex = name % num_philosophers;

        if (!wt.takeIfForkAvailable(leftForkIndex, rightForkIndex))
            return;

        if (!wt.takeIfForkAvailable(rightForkIndex, leftForkIndex, true))
        {
            print("Philosopher ", name, " couldn't take fork #", leftForkIndex, ", releasing fork #", rightForkIndex, ".\n\n");
            wt.putFork(leftForkIndex);
            return;
        }

        print("Philosopher ", name, " is dining.\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        wt.putFork(leftForkIndex);
        wt.putFork(rightForkIndex);

        print("Philosopher ", name, " finished dining.\n");
    }

};


int main()
{
    srand (time(NULL));
    const std::size_t num_philosophers = 5;
    std::vector<Fork> forks(num_philosophers);
    Waiter wt(forks);
    std::vector<Philosopher> philosophers;

    for (int i = 0; i < num_philosophers; ++i)
        philosophers.emplace_back(i + 1, i, (i + 1) % num_philosophers, num_philosophers, wt);

    std::vector<std::thread> threads;
    for (auto& philosopher : philosophers)
        threads.emplace_back(&Philosopher::action, &philosopher);

    for (auto& thread : threads)
        thread.join();
}

