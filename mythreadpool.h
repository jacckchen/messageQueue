//
// Created by 13717 on 2025/6/18.
//

#ifndef LIST_MYTHREADPOOL_H
#define LIST_MYTHREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>


class ThreadPool {

    ThreadPool(size_t size);
    ~ThreadPool();
    template<class F,class ...Args>
    auto enqueue(F&& f,Args&& ...args)
    -> std::future<typename std::result_of<F(Args... args)>::type>;
private:
    std::vector<std::thread> worker;
    std::queue<std::function<void()>> worklist;
    bool stop;
    std::mutex queuemtx;
    std::condition_variable conditon;
};
template<class F,class ...Args>
auto ThreadPool::enqueue(F&&f, Args&&...args)
->std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type=typename std::result_of<F(Args...)>::type;

    auto task=std::make_shared< std::packaged_task<return_type()> > (
        std::bind(std::forward<F>(f),std::forward<Args>(args)...)
    );
    std::future<return_type> res=task.get_future();
    {
        std::unique_lock<std::mutex> lock(this->queuemtx);
        
        if(stop)
            throw std::runtime_error("it has stopped");
        worklist.emplace([task](){(*task)();});
    }
    conditon.notify_one();
    return res;
}
ThreadPool::ThreadPool(size_t size):stop(false){
    this->worker.reserve(size);
    for(size_t i=0;i<size;i++)
    {
        worker.emplace_back([this]{
                while (true)
                {
                    std::function<void()>task;
                    {
                        std::unique_lock<std::mutex> lock(this->queuemtx);
                        this->conditon.wait(lock,[this]()->bool { return !this->worklist.empty()||this->stop;});
                        if(worklist.empty()&&stop)return;
                        task=worklist.front();
                        worklist.pop();
                    }
                    task();
                }
            });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock lck(this->queuemtx);
        stop=true;
    }
    conditon.notify_all();
    for(auto &task:worker)
    {
        task.join();
    }
}

#endif //LIST_MYTHREADPOOL_H
