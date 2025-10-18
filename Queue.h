//
// Created by 13717 on 2025/10/1.
//
#pragma once
#include <mutex>
#include <condition_variable>
#include <list>
#include <thread>
template<class T>
class Queue {
public:
    std::list<T> theList;
    std::mutex mtx2;
    std::condition_variable cv;

    void enQueue_back(const T &x) {
        {
            std::unique_lock<std::mutex> lock(this->mtx);
            std::unique_lock<std::mutex> lock2(this->mtx2);
            this->theList.push_back(x);
        }
        this->cv.notify_all();
    }

    void Erase(typename std::list<T>::iterator it) {

        {
            std::unique_lock<std::mutex> lock(this->mtx);
            this->theList.erase(it);
        }
    }

    T deQueue_front() noexcept{
        if (this->Empty())
            return nullptr;
        {
            std::unique_lock<std::mutex> lock(this->mtx);
            T x = theList.front();
//            cout<<"get x"<<x;
            this->theList.pop_front();
            return x;
        }
    }

    bool Empty() {
        {
            std::unique_lock<std::mutex> lock(this->mtx);
            return this->theList.empty();
        }
    }

private:
    std::mutex mtx;
};


