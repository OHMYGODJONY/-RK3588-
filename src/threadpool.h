#pragma once
 
#include <iostream>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <atomic>
 
namespace tdpool
{
 
    const int MAX_TASK_NUM = 5; 
    const int MAX_THREAD_NUM = 5;
    const int THREAD_MAX_IDLE_TIME = 60; 

 
    enum class PoolMode
    {
        kFIXED = 0,
        kCached,
    };
 
    class Thread
    {
    public:
        using Func = std::function<void(int)>;
        Thread(Func func)
        :func_(func),
        thread_id_(generate_ID_++)
        {}
 
        ~Thread() = default;
 
        void start()
        {
            std::thread t(func_,thread_id_);
            std::cout << "thread: " << thread_id_ << " start" << std::endl;
            t.detach();
        }
 
        int getID() const
        {
            return thread_id_;
        }
 
    private:
        Func func_;
        static int generate_ID_;
        int thread_id_ ;
    };
    
    // int Thread::generate_ID_ = 0;
    class ThreadPool
    {
    public:
        ThreadPool():
        init_thread_num_(0),
        max_thread_num_(MAX_THREAD_NUM),
        idle_thread_num_(0),
        task_num_(0),
        max_task_num_(MAX_TASK_NUM),
        pool_mode_(PoolMode::kFIXED),
        is_running_(false)
        {}
 
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
 
        ~ThreadPool()
        {
            is_running_ = false;
            std::unique_lock<std::mutex> lock(mtx_);
            not_empty_.notify_all();
            exit_cond_.wait(lock,[&]()->bool{return threads_.empty();});
        }
 
        void setMode(PoolMode mode)
        {
            if (checkrunningstate())
                return;
            pool_mode_ = mode;      
        }
 
        void setMaxTaskNum(int threshhold)
        {
            if (checkrunningstate())
                return;
            max_task_num_ = threshhold;
        }
 
        void setThreadSizeThreshHold(int threshhold)
        {
            if (checkrunningstate())
                return;
            if (pool_mode_ == PoolMode::kCached)
            {
                max_thread_num_ = threshhold;
            }
        }
 
        template<typename Func, typename... Args>
        auto submitTask(Func&& func,Args&&... args)->std::future<decltype(func(args...))>
        {
            using Rtype = decltype(func(args...));
            auto task = std::make_shared<std::packaged_task<Rtype()>>(std::bind(std::forward<Func>(func),std::forward<Args>(args)...));
            std::future<Rtype> result = task->get_future();
 
            std::unique_lock<std::mutex> lock(mtx_);
            if(!not_full_.wait_for(lock,std::chrono::seconds(1),[&]()->bool{return task_que_.size() < (size_t)max_task_num_;}))
            {
                std::cerr << "task queue is full, submit task fail." << std::endl;
                auto task = std::make_shared<std::packaged_task<Rtype()>>([]()->Rtype { return Rtype(); });
                (*task)();
                return task->get_future();
            }
            task_que_.emplace([task](){
                (*task)();
            });
            not_empty_.notify_all();
 
            if(pool_mode_ == PoolMode::kCached && task_num_ > idle_thread_num_ && threads_.size() < max_thread_num_)
            {
                auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
                int threadid = ptr->getID();
                threads_.emplace(threadid,std::move(ptr));
                threads_[threadid]->start();
                idle_thread_num_++;
            }
            return result;
        }   
 
        void start(int init_thread_num = std::thread::hardware_concurrency())
        {
            init_thread_num_ = init_thread_num;
            is_running_ = true;
 
            for(int i = 0; i < init_thread_num_; i++)
            {
                auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
                int threadid = ptr->getID();
                threads_.emplace(threadid,std::move(ptr));
                threads_[threadid]->start();
                idle_thread_num_++;
            }
        }
 
    private:
        bool checkrunningstate() const
        {
            return is_running_;
        }
 
        void threadFunc(int threadid)
        {
            auto last_time = std::chrono::high_resolution_clock().now();
            while(1)
            {
                Task task;
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    while(task_que_.empty())
                    {
                        if(!is_running_)
                        {
                            threads_.erase(threadid);
                            exit_cond_.notify_all();
                            return;
                        }
                        if(pool_mode_ == PoolMode::kCached)
                        {
                            if(std::cv_status::timeout == not_empty_.wait_for(lock,std::chrono::seconds(1)))
                            {
                                auto now = std::chrono::high_resolution_clock().now();
                                auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);
                                if(dur.count() >= THREAD_MAX_IDLE_TIME && threads_.size() > init_thread_num_)
                                {
                                    threads_.erase(threadid);
                                    idle_thread_num_--;
                                    exit_cond_.notify_all();
                                    return;
                                }
                            }
                        }
                        else
                        {
                            not_empty_.wait(lock);
                        }
                    }
                    task = task_que_.front();
                    task_que_.pop();
                    task_num_--;
                    idle_thread_num_--;
                    if(task_que_.size() > 0)
                    {
                        not_empty_.notify_all();
                    }
                    not_full_.notify_all();
                }
                if(task)
                {
                    task();
                }
                idle_thread_num_++;
                last_time = std::chrono::high_resolution_clock().now();
            }
        }
 
    private:
        using ThreadPtr = std::unique_ptr<Thread>;
        std::unordered_map<int,ThreadPtr> threads_;
        int init_thread_num_;
        int max_thread_num_; 
        std::atomic_int idle_thread_num_; 
 
        using Task = std::function<void()>;
        std::queue<Task> task_que_;
        std::atomic_int task_num_;
        int max_task_num_;
        
        std::mutex mtx_;
        std::condition_variable not_full_;
        std::condition_variable not_empty_;
        std::condition_variable exit_cond_;
 
        PoolMode pool_mode_;
        std::atomic_bool is_running_;
    };
}
 
 