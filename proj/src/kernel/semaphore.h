//
// Created by Stanislav Král on 14.10.2020.
#pragma once

#include <mutex>
#include <condition_variable>
#include <Windows.h>

class Semaphore {
public:
    explicit Semaphore(size_t count_) {
        count = count_;
    }

    inline void notify()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    inline void wait()
    {
        std::unique_lock<std::mutex> lock(mtx);
        while(count == 0){
            cv.wait(lock);
        }
        count--;
    }

    std::mutex mtx; // needs to be mutable
private:

    std::condition_variable cv;
    size_t count;
};
