// This needs to be built with -DIYFT_ENABLE_PROFILING (and, optionally, -DIYFT_THREAD_POOL_PROFILE), e.g, on Linux:
//
// clang++ -pthread -std=c++17 -Wall -Wextra -pedantic -DIYFT_ENABLE_PROFILING -DIYFT_THREAD_POOL_PROFILE MinimalTest.cpp
// OR
// g++ -pthread -std=c++17 -Wall -Wextra -pedantic -DIYFT_ENABLE_PROFILING -DIYFT_THREAD_POOL_PROFILE MinimalTest.cpp

#include <iostream>
#include <string>

// Add the profiler and designate this cpp file as the implementation
#define IYFT_THREAD_PROFILER_IMPLEMENTATION
#include "ThreadProfiler.hpp"

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
    // This will automatically stop the recording.
    std::cout << IYFT_PROFILER_RESULT_STRING << "\n";
    
    return 0;
}
