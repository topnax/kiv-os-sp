//
// Created by Stanislav Král on 14.10.2020.
#pragma once

#include <mutex>
#include <condition_variable>
#include <Windows.h>

// TODO move parts of the class to a semaphore.cpp

class Semaphore {
public:
    Semaphore(int count_, int id_) {
        count = count_;
        id = id_;
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

    int get_id()  {
        return id;
    }

private:
    mutable std::mutex mtx; // needs to be mutable

    std::condition_variable cv;
    int count;
    int id;
};
