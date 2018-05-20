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

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <chrono>

#include "ThreadProfiler.hpp"
#include "ThreadPool.hpp"

#ifdef __linux__
#include <pthread.h>
#elif defined _WIN32
// TODO what to do
#endif

const std::size_t IterationCount = 5;

using namespace std::chrono_literals;

/// A function used for demo purposes.
void subSleeper() {
    /// Using an explicitly set tag
    IYF_PROFILE(subSleeper, iyft::ProfilerTag::NoTag);
}

/// A function used for demo purposes.
void sleeper(std::chrono::nanoseconds ns) {
    /// Implicitly using iyft::ProfilerTag::NoTag as a tag
    IYF_PROFILE(sleeper);
    std::this_thread::sleep_for(ns);
    
    subSleeper();
}

/// A function used for demo purposes.
std::size_t sleepingAnswer(std::chrono::nanoseconds ns, bool correctAnswer) {
    /// Implicitly using iyft::ProfilerTag::NoTag as a tag
    IYF_PROFILE(sleepingAnswer);
    std::this_thread::sleep_for(ns);
    
    return correctAnswer ? 42 : 12345;
}

/// Tracks total time to show how much time we've saved thanks to the thread pool.
std::chrono::milliseconds expectedTime(0);
std::chrono::milliseconds incrementExpected(std::chrono::milliseconds duration) {
    expectedTime += duration;
    return duration;
}

int main() {
    // Explicitly name a thread. You should call this at the start. Otherwise, some
    // other function may assign it a default name and id.
    // 
    // This is identical to the IYF_PROFILER_NAME_THREAD(a) macro and may be used
    // even if profiling is disabled.
    const bool assigned = iyft::AssignThreadName("MAIN");
    
    // Identical to IYF_PROFILER_GET_CURRENT_THREAD_ID
    const std::size_t threadID = iyft::GetCurrentThreadID();
    
    // Identical to IYF_PROFILER_GET_CURRENT_THREAD_NAME
    const char* threadName = iyft::GetCurrentThreadName();
    
    IYF_ASSERT(assigned);
    IYF_ASSERT(threadID == 0);
    IYF_ASSERT(std::strcmp("MAIN", threadName) == 0);
    
    std::cout << "Main thread name assigned? " << std::boolalpha << assigned << 
                 "\nID: " << threadID <<
                 "\nName: " << threadName << "\n\n";
    
    // Starts (if true) of stops (if false) the recording.
    IYF_PROFILER_SET_RECORDING(true)
    
    // Obtain the status of the profiler
    switch (IYF_PROFILER_STATUS) {
        // iyft::ProfilerStatus::Disabled will be returned if IYF_ENABLE_PROFILING
        // hasn't been defined.
        case iyft::ProfilerStatus::Disabled:
            std::cout << "PROFILER: disabled\n\n";
            break;
        case iyft::ProfilerStatus::EnabledAndNotRecording:
            std::cout << "PROFILER: enabled, not recording\n\n";
            break;
        case iyft::ProfilerStatus::EnabledAndRecording:
            std::cout << "PROFILER: enabled, recording\n\n";
            break;
    }
    
// A check to make sure we don't get errors in ThreadPool only builds
#ifdef IYF_ENABLE_PROFILING
    IYF_ASSERT(IYF_PROFILER_STATUS == iyft::ProfilerStatus::EnabledAndRecording);
#endif // IYF_ENABLE_PROFILING
    
    // Tracks total time
    const auto start = std::chrono::high_resolution_clock::now();
    
    // A lambda used for demo purposes.
    const auto sleeperLambda = [](std::chrono::nanoseconds ns){
        std::this_thread::sleep_for(ns);
    };
    
    // Creates a new thread pool with std::thread::hardware_concurrency() workers.
    // You may also assign the number explicitly by using a different constructor.
    //
    // The setup function (lambda in this case) runs in each thread and can be
    // used to assign custom names, priorities, core affinities, etc.
    std::unique_ptr<iyft::ThreadPool> pool = std::make_unique<iyft::ThreadPool>([](std::size_t total, std::size_t current) {
        std::stringstream ss;
        ss << "CustomThread" << current << "of" << total;
        IYF_PROFILER_NAME_THREAD(ss.str().c_str());
        
#ifdef __linux__
        std::cout << "Setting up worker thread " << current << " of " << total
                  << "\nNative handle: " << pthread_self()
                  << "\nID: " << IYF_PROFILER_GET_CURRENT_THREAD_ID
                  << "\nName: " << IYF_PROFILER_GET_CURRENT_THREAD_NAME << "\n\n";
#elif defined _WIN32
        //TODO what to do
#endif
    });
    
    // Validates the worker count
    const std::size_t threadCount = pool->getWorkerCount();
    IYF_ASSERT(threadCount == std::thread::hardware_concurrency());
    
    // "Simulates" frames
    for (std::size_t i = 0; i < IterationCount; ++i) {
        pool->addTaskWithResult(sleepingAnswer, incrementExpected(4ms), true);
        
        pool->addTask(sleeper, incrementExpected(2ms));
        pool->addTask(sleeperLambda, incrementExpected(1ms));
        
        auto resultFuture = pool->addTaskWithResult(sleepingAnswer, incrementExpected(8ms), true);
        
        iyft::Barrier bar1(3);
        pool->addTask(bar1, sleeper, incrementExpected(2ms));
        pool->addTask(bar1, sleeperLambda, incrementExpected(6ms));
        pool->addTask(bar1, sleeper, incrementExpected(2ms));
        
        // Blocks the current thread until all tasks that use bar1 complete.
        auto barrierStart = std::chrono::high_resolution_clock::now();
        bar1.waitForAll();
        auto barrierEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> barrierDuraion = barrierEnd - barrierStart;
        std::cout << "Waiting on a barrier took: " << barrierDuraion.count() << "ms\n";
        
        // Blocks the current thread until the answer gets returned.
        std::size_t theAnswer = resultFuture.get();
        IYF_ASSERT(theAnswer == 42);
        
        pool->addTask(sleeperLambda, incrementExpected(5ms));
        
        pool->addTask(sleeper, incrementExpected(2ms));
        pool->addTask(sleeperLambda, incrementExpected(1ms));
        
        if (i != IterationCount - 1) {
            pool->waitForAll();
            
            // Tells the profiler to assign an end time to the current frame and start
            // a new one.
            IYF_PROFILER_NEXT_FRAME
        } else {
            // Just to prove things work properly when we start requesting results with tasks
            // still in flight.
            std::cout << "Skipping wait on the last frame.\n";
        }
    }
    
// A check to make sure we don't get errors in ThreadPool only builds
#ifdef IYF_ENABLE_PROFILING 
    // You may also use IYF_PROFILER_RESULTS_TO_FILE("profilerResults") to write the results to a file
    // or IYF_PROFILER_RESULT_STRING to write them to an std::string.
    //
    // This function uses Spinlocks, std::swap, etc. internally to return the current results and clear
    // the data buffers as quickly as possible. If you recorded a ton of data, you may also wish to run
    // this function on a separate thread. However, make sure that recording DOES NOT GET ENABLED until 
    // this function is done or you may get very scrambled eggs for breakfast.
    auto resultStart = std::chrono::high_resolution_clock::now();
    iyft::ProfilerResults results = iyft::GetThreadProfiler().getResults();
    auto resultEnd = std::chrono::high_resolution_clock::now();
#endif // IYF_ENABLE_PROFILING 
    
    // Make sure to finish all tasks
    pool = nullptr;
    
    std::cout << "\nALL TASKS COMPLETE; pool closed\n\n";
    
    // Check the total run time.
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> duration = end - start;
    
    // Print the results
    std::cout << "USED " << threadCount << " thread(s)\n";
    std::cout << "COMPLETED WORK IN " << std::fixed << duration.count() << "ms\n";
    std::cout << "Single thread would have taken " << expectedTime.count() << "ms\n";
    std::cout << "Improvement: " << expectedTime.count() / duration.count() << " x\n";
    
// A check to make sure we don't get errors in ThreadPool only builds
#ifdef IYF_ENABLE_PROFILING
    std::chrono::duration<double, std::milli> resultDuraion = resultEnd - resultStart;
    std::cout << "Result extraction took: " << resultDuraion.count() << "ms\n";
    
    // Now that we're done measuring, output the results to a file and try to read them.
    std::cout << std::boolalpha;
    std::cout << "Result write succeeded? " << results.writeToFile("profilerResults.profres") << "\n";
    std::optional<iyft::ProfilerResults> loadedResults = iyft::ProfilerResults::LoadFromFile("profilerResults.profres");
    
    if (!loadedResults) {
        std::cout << "Failed to load results from file\n";
        IYF_ASSERT(false);
    } else {
        // Check to make sure serialization and deserialization work
        IYF_ASSERT(*loadedResults == results);
        std::cout << (*loadedResults).writeToString() << "\n";
    }
#endif // IYF_ENABLE_PROFILING 
    
    return 0;
}
