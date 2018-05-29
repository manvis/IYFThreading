cmake_minimum_required(VERSION 3.0)
project(IYFThreadPool)

if(NOT CMAKE_BUILD_TYPE)
    # To make sure all asserts fire
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("..")
message(STATUS "SOURCE DIR: ${CMAKE_SOURCE_DIR}")


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(USE_THREAD_SANITIZER "TRUE" CACHE BOOL "Should the thread sanitizer be used or not")
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Wconversion -pedantic -fdiagnostics-color=always")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -Wl,--export-dynamic")

    if (${USE_THREAD_SANITIZER})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=thread")
        message(STATUS "Thread sanitizer IS ON")
    else()
        message(STATUS "Thread sanitizer IS OFF")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSCV")
# TODO I barely know anything about MSVC. what should I do here?
endif()

add_definitions("-DIYFT_ENABLE_PROFILING -DIYFT_THREAD_POOL_PROFILE")
find_package(Threads REQUIRED)

add_executable(threadPoolTest Test.cpp Implementation.cpp)
target_link_libraries(threadPoolTest Threads::Threads)
