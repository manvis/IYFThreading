# The IYFThreading library

This is a small, header only C++17 library that contains a thread pool and a thread profiler. It is licensed under the [3-clause BSD] license and depends only on the standard library. Last but not least, when testing, this library is built with -fsanitize=thread and the sanitizer does not detect any invalid behaviour.

This library consists of a couple components that, for the most part, may be used independently of one another:

1. **ThreadPool.hpp**: independent of other headers, unless you enable thread pool profiling. The thread pool distributes assigned work to multiple worker threads. It supports barriers (blocking until specified tasks complete) and tasks with or without returned results.
2. **ThreadProfiler.hpp**:  depends on the spinlock header. Allows you to assign thread names and to retrieve constant sequential IDs for threads even if profiling is disabled. Enables you to record the durations of specified scopes with the help of instrumentation macros. The results may be written to a human readable string, a binary file or displayed immediately.
3. **ThreadProfilerSettings.hpp**: customizable settings and values, specific to your project. Make sure to not accidentally overwrite this file when updating this library.
4. **Spinlock.hpp**: a really basic atomic_flag based spinlock that anyone can write in a minute. Independent of other headers.

This library is still being tested and refined. Moreover, a couple important features (e.g., drawing results with the help of imgui) haven't been implemented yet.

## Configuration
Two macros need to be defined *(or not, if you want to disable the features)* globally, e.g., in your CMake file:

1. ```IYF_ENABLE_PROFILING```

  **Defining** this macro will **enable profiling**. Even if you don't record anything, having profiling enabled will introduce some overhead to your code.

  If this macro **isn't defined**, most ```IYF_PROFILER``` macros will **do nothing** or return constant **empty values**. Moreover, most of the profiler's functions and classes won't even be compiled.

2. ```IYF_THREAD_POOL_PROFILE```

  **Defining** this macro **enables profiling of the thread pool**. If this macro is defined, but ```IYF_ENABLE_PROFILING``` isn't, profiling will still be disabled. Moreover, defining this macro will force **ThreadPool.hpp** to include **ThreadProfiler.hpp**.

Other options only apply to the thread profiler. They are documented in the **ThreadProfilerSettings.hpp** header and should be adjusted there. You should also use the said header to define custom scope tags, names and colours, suitable for your application.

## Usage

```cpp
// This needs to be built with -DIYF_ENABLE_PROFILING (and, optionally, -DIYF_THREAD_POOL_PROFILE), e.g, on Linux:
//
// clang++ -pthread -std=c++17 -Wall -Wextra -pedantic -DIYF_ENABLE_PROFILING -DIYF_THREAD_POOL_PROFILE MinimalTest.cpp
// OR
// g++ -pthread -std=c++17 -Wall -Wextra -pedantic -DIYF_ENABLE_PROFILING -DIYF_THREAD_POOL_PROFILE MinimalTest.cpp

#include <iostream>
#include <string>

// Add the profiler and designate this cpp file as the implementation
#define IYF_THREAD_PROFILER_IMPLEMENTATION
#include "ThreadProfiler.hpp"

// Add the pool AFTER the profiler if IYF_THREAD_POOL_PROFILE is defined
#include "ThreadPool.hpp"

#define FRAME_COUNT 5

std::string withResult(std::size_t num, const std::string& str) {
    IYF_PROFILE(TaskWithResult, iyft::ProfilerTag::NoTag);
    
    return str + std::to_string(num);
}

int main() {
    // Name this thread (optional - a name will be assigned by default otherwise)
    IYF_PROFILER_NAME_THREAD("Main");
    
    // Start the recording
    IYF_PROFILER_SET_RECORDING(true);
    
    // Create a thread pool with std::thread::hardware_concurrency() workers.
    iyft::ThreadPool pool;
    
    // This string needs to be kept alive until we're done with it.
    const std::string test = "test";
    
    for (std::size_t i = 0; i < FRAME_COUNT; ++i) {
        // Add a task from a function and obtain a future.
        auto result = pool.addTaskWithResult(withResult, i, test);
        
        // Add a task lambda with no result
        pool.addTask([](){
            IYF_PROFILE(TaskWithoutAResult, iyft::ProfilerTag::NoTag);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        });
        
        // Busy wait until all jobs are done
        pool.waitForAll();
        
        // This should happen immediately because we waited for all tasks
        IYF_ASSERT(result.get() == (test + std::to_string(i)));
        
        // Mark the end of the frame and start a new one
        IYF_PROFILER_NEXT_FRAME
    }
    
    // Write the results to an std::string and std::cout
    // This will automatically stop the recording.
    std::cout << IYF_PROFILER_RESULT_STRING << "\n";
    
    return 0;
}
```

[3-clause BSD]: https://github.com/manvis/IYFThreading/blob/master/LICENSE
