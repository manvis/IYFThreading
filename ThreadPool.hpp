// The IYFThreading library
//
// Copyright (C) 2018, Manvydas Šliamka
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
// conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
// of conditions and the following disclaimer in the documentation and/or other materials
// provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of other contributors may be
// used to endorse or promote products derived from this software without specific prior
// written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
// WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// \file ThreadPool.hpp Contains the main ThreadPool class

#ifndef IYFT_THREAD_POOL_HPP
#define IYFT_THREAD_POOL_HPP

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <iostream>

#if __cplusplus >= 201703L
#define IYFT_HAS_CPP17
#endif

#ifdef IYFT_THREAD_POOL_PROFILE
#include "ThreadProfiler.hpp"
#endif // IYFT_THREAD_POOL_PROFILE

/// \brief The main namespace of the IYFThreading library
namespace iyft {
class ThreadPool;

/// \brief A barrier that will block until all tasks complete.
class Barrier {
public:
    /// \brief Create a barrier that will block until taskCount tasks complete.
    ///
    /// \throws std::logic_error if taskCount < 0.
    ///
    /// \param taskCount The number of tasks to block for. Must be >= 0.
    Barrier(int taskCount) : taskCount(taskCount) {
        if (taskCount < 0) {
            throw std::logic_error("You must use a non-negative integer for taskCount");
        }
    }
    
    /// \brief Explicitly disabled to get cleaner errors.
    Barrier(const Barrier&) = delete;
    
    /// \brief Explicitly disabled to get cleaner errors.
    Barrier& operator=(const Barrier&) = delete;
    
    /// \brief Explicitly disabled to get cleaner errors.
    Barrier(Barrier&&) = delete;
    
    /// \brief Explicitly disabled to get cleaner errors.
    Barrier& operator=(Barrier&&) = delete;
    
    /// \brief This function will block the calling thread until all tasks complete.
    ///
    /// \warning This function will cause a deadlock if you add less than taskCount 
    /// tasks that use this barrier to the ThreadPool.
    void waitForAll() {
        std::unique_lock<std::mutex> lock(counterMutex);
        barrierCondition.wait(lock, [this]{
            return taskCount == 0;
        });
    }
private:
    friend class ThreadPool;
    
    /// \brief Called by the ThreadPool to notify that the task finished executing.
    void notifyCompleted() {
        {
            std::lock_guard<std::mutex> lock(counterMutex);
            taskCount--;
            
            if (taskCount < 0) {
                throw std::runtime_error("Too many completed task notifications. Did you set the correct task count when creating the barrier?");
            }
        }
        
        barrierCondition.notify_one();
    }
    
    /// \brief The number of tasks to block for.
    int taskCount;
    
    /// \brief A mutex that protects the counter.
    std::mutex counterMutex;
    
    /// \brief A condition variable that's used for waiting.
    std::condition_variable barrierCondition;
};

/// \brief A class that assigns work to multiple threads.
class ThreadPool {
public:
    /// \brief A type to use for a SetupFunction.
    ///
    /// A function with this signature is called by each thread during setup. The 
    /// first parameter is the total number of threads and the second is the number of
    /// the current thread.
    ///
    /// You may use the SetupFunction to assign thread priorities, core affinities, 
    /// etc. (e.g., by using pthread_self() and pthread_setschedparam()).
    ///
    /// Moreover, you can use the function to assign names to the threads by using
    /// IYFT_PROFILER_NAME_THREAD(x).
    using SetupFunction = std::function<void(std::size_t, std::size_t)>;
    
    /// \brief Creates a ThreadPool with std::thread::hardware_concurrency() - 1 workers.
    ///
    /// std::thread::hardware_concurrency() - 1 is used because the main thread that
    /// creates the pool is also a thread and will likely keep doing work as well.
    ///
    /// \remark If std::thread::hardware_concurrency() <= 1, the pool will be instantiated
    /// with a single worker.
    ///
    /// \param setupFunction An optional function that can be used to setup the
    /// threads (e.g., set priorities and/or core affinities using native handles,
    /// set custom thread names, etc.).
    inline ThreadPool(SetupFunction setupFunction = &DefaultSetupFunction)
        : ThreadPool(DetermineWorkerCount(std::thread::hardware_concurrency()), setupFunction) {}
    
    /// \brief Creates a ThreadPool with the specified number of workers.
    ///
    /// \param workerCount The number of workers to create. Must be > 0
    /// \param setupFunction An optional function that can be used to setup the
    /// threads (e.g., set priorities and/or core affinities using native handles,
    /// set custom thread names, etc.).
    inline ThreadPool(std::size_t workerCount, SetupFunction setupFunction = &DefaultSetupFunction)
        : tasksInFlight(0), running(true) {
        if (workerCount == 0) {
            throw std::logic_error("workerCount must be > 0");
        }
        
        workers.reserve(workerCount);
        
        for (std::size_t i = 0; i < workerCount; ++i) {
            workers.emplace_back(&ThreadPool::executeTasks, this, workerCount, i, setupFunction);
        }
    }
    
    /// \brief Explicitly disabled to get cleaner errors.
    ThreadPool(const ThreadPool&) = delete;
    /// \brief Explicitly disabled to get cleaner errors.
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    /// \brief Explicitly disabled to get cleaner errors.
    ThreadPool(ThreadPool&&) = delete;
    /// \brief Explicitly disabled to get cleaner errors.
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(taskMutex);
            running = false;
        }
        
        // Make sure that all threads that have already finished their work wake up
        // and exit so that we could join them.
        newTaskNotifier.notify_all();
        
        for (auto& w : workers) {
            w.join();
        }
    }
    
    /// \brief The default thread setup function that does nothing.
    static void DefaultSetupFunction(std::size_t, std::size_t) {}
    
    /// \brief Returns the number of the threads in the pool.
    ///
    /// \return The number of threads.
    inline std::size_t getWorkerCount() const {
        return workers.size();
    }
    
    /// \brief Returns the number of tasks remaining in the queue.
    ///
    /// \return The number of tasks.
    inline std::size_t getRemainingTaskCount() const {
        std::unique_lock<std::mutex> lock(taskMutex);
        return tasks.size();
    }
    
    /// \brief Adds a task that returns nothing.
    template<typename F, typename... Args>
    inline void addTask(F&& f, Args&&... args) {
#ifdef IYFT_THREAD_POOL_PROFILE
        IYFT_PROFILE(AddTaskNoResultNoBarrier);
#endif // IYFT_THREAD_POOL_PROFILE

        {
            std::lock_guard<std::mutex> lock(taskMutex);
            
            checkRunning();
            
            // If I recall correctly, assigning a packaged_task that returns a 
            // non-void to one that does invokes undefined behaviour.
            auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            tasks.emplace([func](){
                func();
            });
        }
        
        newTaskNotifier.notify_one();
    }
    
    /// \brief Adds a task that returns nothing and notifies a barrier upon 
    /// completion.
    template<typename F, typename... Args>
    inline void addTask(Barrier& barrier, F&& f, Args&&... args) {
#ifdef IYFT_THREAD_POOL_PROFILE
        IYFT_PROFILE(AddTaskNoResultWithBarrier);
#endif // IYFT_THREAD_POOL_PROFILE
        {
            std::lock_guard<std::mutex> lock(taskMutex);
            
            checkRunning();
            
            auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            tasks.emplace([func, &barrier](){
                func();
                barrier.notifyCompleted();
            });
        }
        
        newTaskNotifier.notify_one();
    }
    
    /// \brief Adds a task that returns a future.
    template<typename F, typename... Args>
#ifdef IYFT_HAS_CPP17
    std::future<std::invoke_result_t<F, Args...>> addTaskWithResult(F&& f, Args&&... args) {
#else // IYFT_HAS_CPP17
     std::future<typename std::result_of<F&&(Args&&...)>::type> addTaskWithResult(F&& f, Args&&... args) {
#endif // IYFT_HAS_CPP17
    
#ifdef IYFT_THREAD_POOL_PROFILE
        IYFT_PROFILE(AddTaskWithResultNoBarrier);
#endif // IYFT_THREAD_POOL_PROFILE

#ifdef IYFT_HAS_CPP17
        using ReturnValueType = std::invoke_result_t<F, Args...>;
#else // IYFT_HAS_CPP17
        using ReturnValueType = typename std::result_of<F&&(Args&&...)>::type;
#endif // IYFT_HAS_CPP17
        
        using TaskType = std::packaged_task<ReturnValueType()>;
        
        TaskType task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto taskResult = task.get_future();
        
        {
            std::lock_guard<std::mutex> lock(taskMutex);
            
            checkRunning();
            
            tasks.emplace(std::bind([](TaskType& task){
                task();
            }, std::move(task)));
        }
        
        newTaskNotifier.notify_one();
        return taskResult;
    }
    
    /// \brief Adds a task that returns a future and notifies a barrier upon 
    /// completion.
    template<typename F, typename... Args>
#ifdef IYFT_HAS_CPP17
    std::future<std::invoke_result_t<F, Args...>> addTaskWithResult(Barrier& barrier, F&& f, Args&&... args) {
#else // IYFT_HAS_CPP17
    std::future<typename std::result_of<F&&(Args&&...)>::value> addTaskWithResult(Barrier& barrier, F&& f, Args&&... args) {
#endif // IYFT_HAS_CPP17
#ifdef IYFT_THREAD_POOL_PROFILE
        IYFT_PROFILE(AddTaskWithResultWithBarrier);
#endif // IYFT_THREAD_POOL_PROFILE
        
#ifdef IYFT_HAS_CPP17
        using ReturnValueType = std::invoke_result_t<F, Args...>;
#else // IYFT_HAS_CPP17
        using ReturnValueType = typename std::result_of<F&&(Args&&...)>::type;
#endif // IYFT_HAS_CPP17
        using TaskType = std::packaged_task<ReturnValueType()>;
        
        TaskType task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto taskResult = task.get_future();
        
        {
            std::lock_guard<std::mutex> lock(taskMutex);
            
            checkRunning();
            
            tasks.emplace(std::bind([&barrier](TaskType& task){
                task();
                barrier.notifyCompleted();
            }, std::move(task)));
        }
        
        newTaskNotifier.notify_one();
        return taskResult;
    }

    /// \brief Busily waits until all tasks complete.
    void waitForAll() {
        /// TODO a less strict operation would do.
        while (tasksInFlight.load() != 0) {
            ;
        }
    }
private:
    static std::size_t DetermineWorkerCount(std::size_t i) {
        if (i <= 1) {
            return 1;
        } else {
            return i - 1;
        }
    }
    
    /// \brief Used to check for an invalid state.
    inline void checkRunning() const {
        if (!running) {
            throw std::runtime_error("Cannot add tasks to a pool that's awaiting destruction.");
        }
    }
    
    /// Every single worker in the pool executes this function to acquire new tasks
    /// to work on.
    void executeTasks(std::size_t count, std::size_t current, SetupFunction setup) {
        setup(count, current);
        
#ifdef IYFT_THREAD_POOL_PROFILE
        // Create an ID and a name for this thread. This will do nothing if the user
        // has already set the name in the setup function.
        const std::string name = "PoolWorker" + std::to_string(current);
        iyft::AssignThreadName(name.c_str());
#endif // IYFT_THREAD_POOL_PROFILE
        
        // Don't quit until the destructor tells us to
        while (true) {
            std::packaged_task<void()> activeTask;
            
            {   
#ifdef IYFT_THREAD_POOL_PROFILE
                IYFT_PROFILE(SleepAndAcquireTask)
#endif // IYFT_THREAD_POOL_PROFILE
                std::unique_lock<std::mutex> lock(taskMutex);
                
                newTaskNotifier.wait(lock, [this](){
                    // Stop waiting if we're no longer running or if pending tasks
                    // exist. Return to sleep otherwise.
                    return !(this->running) || !(this->tasks.empty());
                });
                
                // We make sure to finish any remaining tasks before exiting.
                if (!running && tasks.empty()) {
                    break;
                }
                
                // Move the task from the queue and pop its hollow remains
                activeTask = std::move(tasks.front());
                tasks.pop();
            }
        
            // Execute the task in this thread
            tasksInFlight++;
            activeTask();
            tasksInFlight--;
        }
    }
    
    /// \brief A mutex that protects the queue.
    ///
    /// \remark I don't like using mutable, but a mutable mutex is one of few actually
    /// valid cases.
    ///
    /// \todo Perhaps I should look into lock-free queues for lower latency.
    mutable std::mutex taskMutex;
    
    /// \brief The number of tasks that are currently being worked on.
    std::atomic<int> tasksInFlight;
    
    /// \brief A vector that contains all pending tasks
    std::queue<std::packaged_task<void()>> tasks;
    
    /// \brief A condition variable used to notify the workers about newly available
    /// tasks.
    std::condition_variable newTaskNotifier;
    
    /// \brief A vector that contains all launched threads.
    std::vector<std::thread> workers;
    
    /// \brief Used internally to determine if the pool is quitting.
    bool running;
};

}

#endif // IYFT_THREAD_POOL_HPP
