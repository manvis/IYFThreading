// This needs to be built with -DIYFT_ENABLE_PROFILING (and, optionally, -DIYFT_THREAD_POOL_PROFILE), e.g, on Linux:
//
// clang++ -pthread -std=c++11 -Wall -Wextra -pedantic -DIYFT_ENABLE_PROFILING -DIYFT_THREAD_POOL_PROFILE MinimalTest.cpp
// OR
// g++ -pthread -std=c++11 -Wall -Wextra -pedantic -DIYFT_ENABLE_PROFILING -DIYFT_THREAD_POOL_PROFILE MinimalTest.cpp
//
// C++11 is the minimum supported standard version.

#include <iostream>
#include <string>

// Add the profiler and designate this cpp file as the implementation
#define IYFT_THREAD_PROFILER_IMPLEMENTATION
// This is a lightweight header that should be included in the file that will contain
// the implementation and all files that contain functions you want to profile.
#include "ThreadProfiler.hpp"
// This header should only be included in two cases:
// 1. the cpp has IYFT_THREAD_PROFILER_IMPLEMENTATION defined and contains the
// implementation;
// 2. in any files where you need to access, save or process the records.
#include "ThreadProfilerCore.hpp"

// Add the pool AFTER the profiler if IYFT_THREAD_POOL_PROFILE is defined
#include "ThreadPool.hpp"

#define FRAME_COUNT 5

std::string withResult(std::size_t num, const std::string& str) {
    IYFT_PROFILE(TaskWithResult, iyft::ProfilerTag::NoTag);
    
    return str + std::to_string(num);
}

int main() {
    // Name this thread (optional - a name will be assigned by default otherwise)
    IYFT_PROFILER_NAME_THREAD("Main");
    
    // Start the recording
    IYFT_PROFILER_SET_RECORDING(true);
    
    // Create a thread pool with std::thread::hardware_concurrency() - 1 workers.
    iyft::ThreadPool pool;
    
    // This string needs to be kept alive until we're done with it.
    const std::string test = "test";
    
    for (std::size_t i = 0; i < FRAME_COUNT; ++i) {
        // Add a task from a function and obtain a future.
        auto result = pool.addTaskWithResult(withResult, i, test);
        
        // Add a task lambda with no result
        pool.addTask([](){
            IYFT_PROFILE(TaskWithoutAResult, iyft::ProfilerTag::NoTag);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        });
        
        // Busy wait until all jobs are done
        pool.waitForAll();
        
        // This should happen immediately because we waited for all tasks
        IYFT_ASSERT(result.get() == (test + std::to_string(i)));
        
        // Mark the end of the frame and start a new one
        IYFT_PROFILER_NEXT_FRAME
    }
    
    // Write the results to an std::string and std::cout
    // Calling iyft::GetThreadProfiler().getResults() will automatically stop the
    // recording.
    std::cout << iyft::GetThreadProfiler().getResults().writeToString() << "\n";
    
    return 0;
}
