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

/// \file ThreadProfiler.hpp Contains macros, function definitions and a few simple classes
/// mandatory for profiling. This header has to be included 

#ifndef IYFT_THREAD_PROFILER_HPP
#define IYFT_THREAD_PROFILER_HPP

// For std::size_t
#include <cstddef>

// For std::uint8_t
#include <cstdint>

namespace iyft {
class ScopeInfo;

// --- These first few functions may be useful even if profiling is disabled ---

/// \brief Returns a constant ID that corresponds to the calling thread.
///
/// If the calling thread hasn't been assigned a name and ID yet, this function
/// will fetch the next ID from the interval [0; IYFT_THREAD_PROFILER_MAX_THREAD_COUNT)
/// and generate a default name for the thread as well.
///
/// Both values will be assigned to static thread_local variables, therefore,
/// subsequent calls will be cheap and won't require locking.
///
/// \return The ID.
std::size_t GetCurrentThreadID();

/// \brief Returns a name that was assigned to the thread.
///
/// If the calling thread hasn't been assigned a name and ID yet, this function
/// will fetch the next ID from the interval [0; IYFT_THREAD_PROFILER_MAX_THREAD_COUNT)
/// and generate a default name for the thread as well.
///
/// Both values will be assigned to static thread_local variables, therefore,
/// subsequent calls will be cheap and won't require locking.
///
/// \return The name 
const char* GetCurrentThreadName();

/// \brief Returns the total number of registered threads. This function locks a
/// mutex.
///
/// \return The total number of registered threads.
std::size_t GetRegisteredThreadCount();

/// \brief Assigns a name to the current thread.
///
/// If the calling thread **hasn't been assigned a name and ID yet**, this function
/// will fetch the next ID from the interval [0; IYFT_THREAD_PROFILER_MAX_THREAD_COUNT)
/// and assign the provided name to the thread.
///
/// Both values will be assigned to static thread_local variables, therefore,
/// subsequent calls will be cheap and won't require locking.
///
/// \remark Thread names may repeat. Only the ID is unique.
///
/// \param name A pointer to the name string. The contents will be copied. If you
/// pass a nullptr or an empty string, the name will be assigned automatically.
///
/// \return true if the name was assigned successfully and false if the name has
/// already been assigned and your string was ignored.
bool AssignThreadName(const char* name);

// --- End of useful helper functions ---

/*! \brief The result of the IYFT_PROFILER_STATUS macro. */
enum class ProfilerStatus {
    Disabled, /*!< The profiler is disabled. */
    EnabledAndNotRecording, /*!< The profiler is enabled and not recording. */
    EnabledAndRecording /*!< The profiler is enabled and recording. */
};

/// \brief An RGBA color that will be used for scopes tagged with a specific ProfilerTag.
class ScopeColor {
public:
    /// \brief Creates a new ScopeColor instance.
    ///
    /// \param r Red value [0; 255]
    /// \param g Green value [0; 255]
    /// \param b Blue value [0; 255]
    /// \param a Alpha value [0; 255]
    inline ScopeColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
        : r(r), g(g), b(b), a(a) {}
    
    /// \brief Returns the red value [0; 255].
    ///
    /// \return Red value.
    inline std::uint8_t getRed() const {
        return r;
    }
    
    /// \brief Returns the green value [0; 255].
    ///
    /// \return Green value.
    inline std::uint8_t getGreen() const {
        return g;
    }
    
    /// \brief Returns the blue value [0; 255].
    ///
    /// \return Blue value.
    inline std::uint8_t getBlue() const {
        return b;
    }
    
    /// \brief Returns the alpha value [0; 255].
    ///
    /// \return Alpha value.
    inline std::uint8_t getAlpha() const {
        return a;
    }
    
    /// \brief A comparison operator.
    inline friend bool operator==(const ScopeColor& a, const ScopeColor& b) {
        return (a.r == b.r) && (a.g == b.g) && (a.b == b.b) && (a.a == b.a);
    }
private:
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

}

// This is a special file where you can place various settings
#include "ThreadProfilerSettings.hpp"

#ifndef IYFT_THREAD_PROFILER_MAX_THREAD_COUNT
/// \brief The maximum number of threads that the ThreadProfiler will need to track.
///
/// Default value is 16.
///
/// \warning Must be >= 1
#define IYFT_THREAD_PROFILER_MAX_THREAD_COUNT 16
#endif // IYFT_THREAD_PROFILER_MAX_THREAD_COUNT

#ifndef IYFT_THREAD_PROFILER_HASH
/// \brief A hashing function that will be used to make ScopeKey objects.
///
/// Must return a 32 bit integer and take std::string as the parameter.
#define IYFT_THREAD_PROFILER_HASH(a) \
    std::hash<std::string>{}(a)
#endif // IYFT_THREAD_PROFILER_HASH

#if !defined IYFT_THREAD_TEXT_OUTPUT_DURATION || !defined IYFT_THREAD_TEXT_OUTPUT_NAME
/// \brief An std::duration that's used for text output.
#define IYFT_THREAD_TEXT_OUTPUT_DURATION std::chrono::duration<double, std::milli>
/// \brief A string that names the duration. 
#define IYFT_THREAD_TEXT_OUTPUT_NAME "ms"
#endif // !defined IYFT_THREAD_TEXT_OUTPUT_DURATION || !defined IYFT_THREAD_TEXT_OUTPUT_NAME

static_assert(IYFT_THREAD_PROFILER_MAX_THREAD_COUNT >= 1, "IYFT_THREAD_PROFILER_MAX_THREAD_COUNT must be >= 1");

namespace iyft {
/// \brief Marks the start of the next frame.
///
/// \remark You should prefer to use the IYFT_PROFILER_NEXT_FRAME macro.
void MarkNextFrame();

/// \brief Inserts a new scope into the scope map.
///
/// \warning Don't call this manually and use IYFT_PROFILE instead.
ScopeInfo& InsertScopeInfo(const char* scopeName, const char* identifier, const char* functionName, const char* fileName, std::uint32_t line, ProfilerTag tag);

/// \brief Starts monitoring the current scope.
///
/// \warning Don't call this manually and use IYFT_PROFILE instead.
void InsertScopeStart(const ScopeInfo& info);

/// \brief Finishes monitoring the current scope.
///
/// \warning Don't call this manually and use IYFT_PROFILE instead.
void InsertScopeEnd(const ScopeInfo& info);

/// \brief Starts or stops recording.
///
/// \remark You should prefer to use the IYFT_PROFILER_SET_RECORDING macro.
void SetRecording(bool recording);

/// \brief Obtains the current status of the ThreadProfiler.
///
/// \remark You should prefer to use the IYFT_PROFILER_STATUS macro.
ProfilerStatus GetStatus();

/// \brief A class that helps with automatic scope start and end tracking.
class ScopeProfilerHelper {
public:
    /// \brief Starts tracking the scope.
    ScopeProfilerHelper(const ScopeInfo& info) : info(info) {
        InsertScopeStart(info);
    }
    
    /// \brief Finishes tracking the scope.
    ~ScopeProfilerHelper() {
        InsertScopeEnd(info);
    }
private:
    const ScopeInfo& info;
};
}

/// \brief Assigns a name to the current thread.
///
/// Calls iyft::AssignThreadName(), therefore make sure to read the 
/// documentation of the said function.
///
/// \param a The name of the thread. Must be a C string.
///
/// \return If the assignment succeeded.
#define IYFT_PROFILER_NAME_THREAD(a) iyft::AssignThreadName(a)

/// \brief Returns the name of the current thread.
///
/// Calls iyft::GetCurrentThreadName(), therefore make sure to read the 
/// documentation of the said function.
///
/// \return The name of the current thread.
#define IYFT_PROFILER_GET_CURRENT_THREAD_NAME iyft::GetCurrentThreadName()

/// \brief Returns the id of the current thread.
///
/// Calls iyft::GetCurrentThreadID(), therefore make sure to read the 
/// documentation of the said function.
///
/// \return The name of the current thread.
#define IYFT_PROFILER_GET_CURRENT_THREAD_ID iyft::GetCurrentThreadID()

// You should make sure that IYFT_ENABLE_PROFILING is NOT DEFINED in release builds.
// Even if recording is off, profiling adds some overhead.
#ifdef IYFT_ENABLE_PROFILING

/// \brief Stringify the parameter.
///
/// \param a Object to stringify.
#define IYFT_STRINGIFY(a) #a

/// \brief Used to expand and stringify a macro.
///
/// \param a Object to expand and stringify.
#define IYFT_EXPAND_STRINGIFY(a) IYFT_STRINGIFY(a)

/// \brief Used to expand a macro.
///
/// \param a Object to expand
#define IYFT_EXPAND(a) a

/// \brief Concatenates two strings.
///
/// \param a The left side of the final string.
/// \param b The right side of the final string.
#define IYFT_CONCAT(a, b) a ## b

#if defined(__clang__)
#define FUNCTION_NAME_MACRO __PRETTY_FUNCTION__
#elif defined(__GNUC__) || defined(__GNUG__)
#define FUNCTION_NAME_MACRO __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define FUNCTION_NAME_MACRO __FUNCSIG__
#else
#define FUNCTION_NAME_MACRO __func__
#endif

/// \brief Profiles a named scope with a tag.
#define IYFT_PROFILE_2(name, tag) \
static iyft::ScopeInfo& ScopeInfo##name = iyft::InsertScopeInfo(\
    #name,\
    __FILE__ ":" IYFT_EXPAND_STRINGIFY(__LINE__),\
    FUNCTION_NAME_MACRO,\
    __FILE__,\
    __LINE__,\
    tag); \
    iyft::ScopeProfilerHelper ProfiledScope##name(ScopeInfo##name);

/// \brief Profiles a named scope without a tag.
#define IYFT_PROFILE_1(name) \
static iyft::ScopeInfo& ScopeInfo##name = iyft::InsertScopeInfo(\
    #name,\
    __FILE__ ":" IYFT_EXPAND_STRINGIFY(__LINE__),\
    FUNCTION_NAME_MACRO,\
    __FILE__,\
    __LINE__,\
    iyft::ProfilerTag::NoTag); \
    iyft::ScopeProfilerHelper ProfiledScope##name(ScopeInfo##name);

/// \brief Concats the name with an ID to choose an appropriate macro.
#define IYFT_SELECT(NAME, ID) IYFT_CONCAT(NAME##_, ID)

/// \brief Together with IYFT_VA_SIZE helps with macro "overloading".
/// 
/// \remark Based on https://stackoverflow.com/questions/16683146/can-macros-be-overloaded-by-number-of-arguments
/// and https://stackoverflow.com/a/11172679
#define IYFT_GET_COUNT(_1, _2, COUNT, ...) COUNT

/// \brief Together with IYFT_GET_COUNT helps with macro "overloading".
/// 
/// \remark Based on https://stackoverflow.com/questions/16683146/can-macros-be-overloaded-by-number-of-arguments
/// and https://stackoverflow.com/a/11172679
#define IYFT_VA_SIZE(...) IYFT_EXPAND(IYFT_GET_COUNT(__VA_ARGS__, 2, 1, discardMe))

/// \brief A helper that chooses one of several functions.
#define IYFT_PROFILE_MACRO_PICKER(NAME, ...) IYFT_SELECT(NAME, IYFT_VA_SIZE(__VA_ARGS__))(__VA_ARGS__)

/// \brief Depending on the number of parameters, chooses one of profiling macros.
#define IYFT_PROFILE(...) IYFT_PROFILE_MACRO_PICKER(IYFT_PROFILE, __VA_ARGS__)

/// \brief Used to start or stop a recording
///
/// \param a A boolean. If true, starts a recording, if false, stops it.
#define IYFT_PROFILER_SET_RECORDING(a) iyft::SetRecording(a);

/// \brief Starts the next frame.
#define IYFT_PROFILER_NEXT_FRAME iyft::MarkNextFrame();

/// \brief Returns the status of the profiler as a ProfilerStatus value. 
#define IYFT_PROFILER_STATUS iyft::GetStatus()

#else //  defined(IYFT_ENABLE_PROFILING)

/// \brief Depending on the number of parameters, chooses one of profiling macros.
#define IYFT_PROFILE(...) ((void)0);

/// \brief Used to start or stop a recording
///
/// \param a A boolean. If true, starts a recording, if false, stops it.
#define IYFT_PROFILER_SET_RECORDING(a) ((void)0);

/// \brief Starts the next frame.
#define IYFT_PROFILER_NEXT_FRAME ((void)0);

/// \brief Returns the status of the profiler as a ProfilerStatus value. 
#define IYFT_PROFILER_STATUS iyft::ProfilerStatus::Disabled

#endif // IYFT_ENABLE_PROFILING

#endif // IYFT_THREAD_PROFILER_HPP


