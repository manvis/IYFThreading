# The IYFThreading library

This is a small, header only C++17 library that contains a thread pool and a thread profiler. It is licensed under the [3-clause BSD] license and depends only on the standard library. Last but not least, when testing, this library is built with -fsanitize=thread and the sanitizer does not detect any invalid behaviour.

This library consists of a couple components that, for the most part, may be used independently of one another:

1. **ThreadPool.hpp**: independent of other headers, unless you enable thread pool profiling. The thread pool distributes assigned work to multiple worker threads. It supports barriers (blocking until specified tasks complete) and tasks with or without returned results.
2. **ThreadProfiler.hpp**:  depends on the spinlock header. Allows you to assign thread names and to retrieve constant sequential IDs for threads even if profiling is disabled. Enables you to record the durations of specified scopes with the help of instrumentation macros. The results may be written to a human readable string, a binary file or displayed immediately.
3. **ThreadProfilerSettings.hpp**: customizable settings and values, specific to your project. Make sure to not accidentally overwrite this file when updating this library.
4. **Spinlock.hpp**: a really basic atomic_flag based spinlock that anyone can write in a minute. Independent of other headers.

This library is still being tested and refined. Moreover, a couple important features (e.g., drawing results with the help of imgui) haven't been implemented yet.

## Configuration
Two macros need to be defined (or not, if you want to disable the features) globally, e.g., in your CMake file:

1. ```IYFT_ENABLE_PROFILING```

  **Defining** this macro will **enable profiling**. Even if you don't record anything, having profiling enabled will introduce some overhead to your code.

  If this macro **isn't defined**, most ```IYFT_PROFILER``` macros will **do nothing** or return constant **empty values**. Moreover, most of the profiler's functions and classes won't even be compiled.

2. ```IYFT_THREAD_POOL_PROFILE```

  **Defining** this macro **enables profiling of the thread pool**. If this macro is defined, but ```IYFT_ENABLE_PROFILING``` isn't, profiling will still be disabled. Moreover, defining this macro will force **ThreadPool.hpp** to include **ThreadProfiler.hpp**.

Other options only apply to the thread profiler. They are documented in the **ThreadProfilerSettings.hpp** header and should be adjusted there. You should also use the said header to define custom scope tags, names and colours, suitable for your application.

## Documentation
To build the full documentation, use Doxygen with the provided Doxyfile.

## Usage

This is a simple example. For a more complete example that uses more features, check out the contents of the ```examples``` folder.

```cpp
// This needs to be built with -DIYFT_ENABLE_PROFILING (and, optionally, -DIYFT_THREAD_POOL_PROFILE), e.g, on Linux:
//
// clang++ -pthread -std=c++17 -Wall -Wextra -pedantic -DIYFT_ENABLE_PROFILING -DIYFT_THREAD_POOL_PROFILE MinimalTest.cpp
// OR
// g++ -pthread -std=c++17 -Wall -Wextra -pedantic -DIYFT_ENABLE_PROFILING -DIYFT_THREAD_POOL_PROFILE MinimalTest.cpp

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
```

[3-clause BSD]: https://github.com/manvis/IYFThreading/blob/master/LICENSE
