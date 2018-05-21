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

/// \file ThreadProfiler.hpp Contains all classes and macros used for profiling

#ifndef IYF_THREAD_PROFILER_HPP
#define IYF_THREAD_PROFILER_HPP

// For std::size_t
#include <cstddef>
#include <cstdint>

namespace iyft {
// --- These first few functions may be useful even if profiling is disabled ---

/// \brief Returns a constant ID that corresponds to the calling thread.
///
/// If the calling thread hasn't been assigned a name and ID yet, this function
/// will fetch the next ID from the interval [0; IYF_THREAD_PROFILER_MAX_THREAD_COUNT)
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
/// will fetch the next ID from the interval [0; IYF_THREAD_PROFILER_MAX_THREAD_COUNT)
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
/// will fetch the next ID from the interval [0; IYF_THREAD_PROFILER_MAX_THREAD_COUNT)
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

/*! \brief The result of the IYF_PROFILER_STATUS macro. */
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

#ifdef NDEBUG
#define IYF_ASSERT(a) ((void)(a))
#else // NDEBUG
#include <cassert>
/// A modified assert macro that doesn't cause unused variable warnings when asserions are
/// disabled.
#define IYF_ASSERT(a) assert(a);
#endif // NDEBUG

// This is a special file where you can place various settings
#include "ThreadProfilerSettings.hpp"

#ifndef IYF_THREAD_PROFILER_MAX_THREAD_COUNT
/// \brief The maximum number of threads that the ThreadProfiler will need to track.
///
/// Default value is 16.
///
/// \warning Must be >= 1
#define IYF_THREAD_PROFILER_MAX_THREAD_COUNT 16
#endif // IYF_THREAD_PROFILER_MAX_THREAD_COUNT

#ifndef IYF_THREAD_PROFILER_HASH
/// \brief A hashing function that will be used to make ScopeKey objects.
///
/// Must return a 32 bit integer and take std::string as the parameter.
#define IYF_THREAD_PROFILER_HASH(a) \
    std::hash<std::string>{}(a)
#endif // IYF_THREAD_PROFILER_HASH

#if !defined IYF_THREAD_TEXT_OUTPUT_DURATION || !defined IYF_THREAD_TEXT_OUTPUT_NAME
/// \brief An std::duration that's used for text output.
#define IYF_THREAD_TEXT_OUTPUT_DURATION std::chrono::duration<double, std::milli>
/// \brief A string that names the duration. 
#define IYF_THREAD_TEXT_OUTPUT_NAME "ms"
#endif // !defined IYF_THREAD_TEXT_OUTPUT_DURATION || !defined IYF_THREAD_TEXT_OUTPUT_NAME

static_assert(IYF_THREAD_PROFILER_MAX_THREAD_COUNT >= 1);

/// \brief Assigns a name to the current thread.
///
/// Calls iyft::AssignThreadName(), therefore make sure to read the 
/// documentation of the said function.
///
/// \param a The name of the thread. Must be a C string.
///
/// \return If the assignment succeeded.
#define IYF_PROFILER_NAME_THREAD(a) iyft::AssignThreadName(a)

/// \brief Returns the name of the current thread.
///
/// Calls iyft::GetCurrentThreadName(), therefore make sure to read the 
/// documentation of the said function.
///
/// \return The name of the current thread.
#define IYF_PROFILER_GET_CURRENT_THREAD_NAME iyft::GetCurrentThreadName()

/// \brief Returns the id of the current thread.
///
/// Calls iyft::GetCurrentThreadID(), therefore make sure to read the 
/// documentation of the said function.
///
/// \return The name of the current thread.
#define IYF_PROFILER_GET_CURRENT_THREAD_ID iyft::GetCurrentThreadID()

// You should make sure that IYF_ENABLE_PROFILING is NOT DEFINED in release builds.
// Even if recording is off, profiling adds some overhead.
#if defined(IYF_ENABLE_PROFILING)

#include <mutex>
#include <unordered_map>
#include <deque>
#include <vector>
#include <chrono>
#include <optional>
#include <array>
#include <string>
#include "Spinlock.hpp"

namespace iyft {
/// \brief The clock that the profiler will use.
using ProfilerClock = std::chrono::high_resolution_clock;

#ifdef IYF_PROFILER_WITH_COOKIE
using ProfilerCookie = std::uint64_t;
#endif // IYF_PROFILER_WITH_COOKIE

/// \brief A key used to identify profiled scopes in an unordered_map.
class ScopeKey {
public:
    /// \brief Creates a new ScopeKey instance from a provided hash value.
    ///
    /// \param hashValue An unisgned 32 bit hash value.
    inline explicit ScopeKey(std::uint32_t hashValue) : hashValue(hashValue) {}
    
    /// \brief A comparison operator.
    inline friend bool operator==(const ScopeKey& a, const ScopeKey& b) {
        return a.hashValue == b.hashValue;
    }
    
    /// \brief Returns the hashValue that is wrapped by the ScopeKey object.
    ///
    /// \return The hashValue as an unsigned 32 bit integer.
    inline std::uint32_t getValue() const noexcept {
        return hashValue;
    }
private:
    std::uint32_t hashValue;
};
}

namespace std {
/// \brief An std::hash specialization for ScopeKey.
template<>
struct hash<iyft::ScopeKey> {
    /// \brief Returns a hash value from the ScopeKey.
    ///
    /// \returns A hash value from the ScopeKey.
    std::size_t operator()(const iyft::ScopeKey& hash) const noexcept {
        return hash.getValue();
    }
};
}

namespace iyft {
/// \brief Data unique per profiled scope.
class ScopeInfo {
public:
    /// \brief Creates a new ScopeInfo.
    ///
    /// \param key The key of the current scope.
    /// \param name The name of the current scope.
    /// \param functionName The name of the function that the profiled scope resides 
    /// in.
    /// \param fileName The name of the file that the the profiled scope reisdes in.
    /// \param lineNumber The line number of the profiled scope.
    /// \param tag A tag assigned to the profiled scope
    inline ScopeInfo(ScopeKey key, std::string name, std::string functionName, std::string fileName, std::uint32_t lineNumber, ProfilerTag tag)
        : key(key), tag(tag), name(std::move(name)), functionName(std::move(functionName)), fileName(std::move(fileName)), lineNumber(lineNumber) {}
    
    /// \brief Returns the key of this scope.
    ///
    /// \return The key of this scope.
    inline ScopeKey getKey() const {
        return key;
    }
    
    /// \brief The name of the scope that was provided by the user.
    ///
    /// \return The name of the scope.
    inline const std::string& getName() const {
        return name;
    }
    
    /// \brief The name of the function that the profiled scope resides in.
    ///
    /// \return The name of the function that contains the profiled scope 
    inline const std::string& getFunctionName() const {
        return functionName;
    }
    
    /// \brief The name of the file that the the profiled scope reisdes in.
    ///
    /// \return The name of the source file that contains the profiled scope.
    inline const std::string& getFileName() const {
        return fileName;
    }
    
    /// \brief The line number of the profiled scope.
    ///
    /// \return The line number that marks the start of the profiled scope.
    inline std::uint32_t getLineNumber() const {
        return lineNumber;
    }
    
    /// \brief The tag of this scope.
    ///
    /// \return The tag of this scope.
    inline ProfilerTag getTag() const {
        return tag;
    }
    
    /// A comparison operator
    inline friend bool operator==(const ScopeInfo& a, const ScopeInfo& b) {
        return (a.key == b.key) &&
               (a.tag == b.tag) &&
               (a.name == b.name) &&
               (a.functionName == a.functionName) &&
               (a.fileName == b.fileName) &&
               (a.lineNumber == b.lineNumber);
    }
private:
    ScopeKey key;
    ProfilerTag tag;
    std::string name;
    std::string functionName;
    std::string fileName;
    std::uint32_t lineNumber;
};

/// \brief Base class for profiler objects that use timing functionality.
class TimedProfilerObject {
public:
    /// \brief Creates a new TimedProfilerObject.
    ///
    /// \param start The start time as a duration since the clock's epoch.
    TimedProfilerObject(std::chrono::nanoseconds start) 
        : start(start) {}
    
    /// \brief Returns the start time as a duration since the clock's epoch.
    ///
    /// \return The start time as a duration since the clock's epoch.
    inline std::chrono::nanoseconds getStart() const {
        return start;
    }
    
    /// \brief Returns the end time as a duration since the clock's epoch.
    ///
    /// \return The end time as a duration since the clock's epoch.
    inline std::chrono::nanoseconds getEnd() const {
        return end;
    }
    
    /// \brief Sets the end time.
    ///
    /// \param end The end time. Must be a duration since the clock's epoch.
    inline void setEnd(std::chrono::nanoseconds end) {
        this->end = end;
    }
    
    /// \brief Returns the duration of the TimedProfilerObject.
    ///
    /// \warning Using this function when isComplete() is false invokes undefined
    /// behaviour.
    ///
    /// \returns The duration of the TimedProfilerObject.
    inline std::chrono::nanoseconds getDuration() const {
        return end - start;
    }
    
    /// \brief Determines if the TimedProfilerObject is complete.
    ///
    /// It is done by checking if the end time is bigger than the start time.
    ///
    /// \return true if the frame is complete. False otherwise.
    inline bool isComplete() const {
        return start < end;
    }
    
    /// \brief Checks if the start and end values are different.
    ///
    /// \return true if the start and end values are different.
    inline bool isValid() const {
        return start != end;
    }
private:
    std::chrono::nanoseconds start;
    std::chrono::nanoseconds end;
};

/// \brief A record of an event.
class RecordedEvent : public TimedProfilerObject {
public:
    /// \brief Creates a new RecordedEvent object.
    ///
    /// \param key The key of this scope.
    /// \param depth Current stack depth.
    /// \param start The start time of the recorded event as a duration since the
    /// clock's epoch.
    inline RecordedEvent(ScopeKey key, int depth, std::chrono::nanoseconds start) 
        : TimedProfilerObject(start), key(key), depth(depth) {}
    
#ifdef IYF_PROFILER_WITH_COOKIE
    inline ProfilerCookie getCookie() const {
        return cookie;
    }
    
    inline void setCookie(ProfilerCookie cookie) {
        this->cookie = cookie;
    }
#endif // IYF_PROFILER_WITH_COOKIE
    
    /// \brief Returns the key that identifies the scope of this RecordedEvent.
    ///
    /// \return The key that identifies the scope of this RecordedEvent.
    inline ScopeKey getKey() const {
        return key;
    }
    
    /// \brief Returns the stack depth of this RecordedEvent.
    ///
    /// \brief return The stack depth of this RecordedEvent.
    inline std::int32_t getDepth() const {
        return depth;
    }
    
    /// A comparison operator.
    inline friend bool operator<(const RecordedEvent& a, const RecordedEvent& b) {
        return a.getStart() < b.getStart();
    }
    
    /// A comparison operator.
    inline friend bool operator==(const RecordedEvent& a, const RecordedEvent& b) {
        return (a.key == b.key) &&
               (a.depth == b.depth) &&
#ifdef IYF_PROFILER_WITH_COOKIE
               (a.cookie == b.cookie) &&
#endif // IYF_PROFILER_WITH_COOKIE
               (a.getStart() == b.getStart()) &&
               (a.getEnd() == a.getEnd());
    }
private:
    ScopeKey key;
    std::int32_t depth;
#ifdef IYF_PROFILER_WITH_COOKIE
    ProfilerCookie cookie;
#endif // IYF_PROFILER_WITH_COOKIE
};

/// \brief A record of a frame.
class FrameData : public TimedProfilerObject {
public:
    /// \brief Constructs a new FrameData object.
    ///
    /// \param number The number of the frame.
    /// \param start The start time of the frame as a duration since the clock's
    /// epoch.
    FrameData(std::uint64_t number, std::chrono::nanoseconds start) 
        : TimedProfilerObject(start), number(number) {}
    
    /// \brief A comparison operator.
    inline friend bool operator==(const FrameData& a, const FrameData& b) {
        return (a.number == b.number) &&
               (a.getStart() == b.getStart()) &&
               (a.getEnd() == a.getEnd());
    }
    
    /// \brief Returns the number of the frame.
    ///
    /// \return The number of the frame.
    inline std::uint64_t getNumber() const {
        return number;
    }
private:
    /// \brief The number of the current frame.
    std::uint64_t number;
};

class ProfilerResults;

/// \brief The main class that manages and coordinates all profiling and result
/// exporting actions.
class ThreadProfiler {
public:
    /// \brief Creates a new ThreadProfiler instance.
    ThreadProfiler() : recording(false), frameNumber(0) {
        frames.emplace_back(frameNumber, ProfilerClock::now().time_since_epoch());
    }
    
    /// \brief Inserts a new scope.
    ///
    /// \param scopeName The name of the scope. Must be a string literal.
    /// \param identifier An identifier that the hash used by the ScopeKey is
    /// generated from. Must be a string literal.
    /// \param functionName The name of the function. Must be the result of the
    /// __func__ variable.
    /// \param fileName The name of the file. Must be the result of the __FILE__
    /// macro.
    /// \param line The number of the line. Must be the result of the __LINE__ macro.
    /// \param tag The tag of the scope.
    inline ScopeInfo& insertScopeInfo(const char* scopeName, const char* identifier, const char* functionName, const char* fileName, std::uint32_t line, ProfilerTag tag) {
        std::lock_guard<Spinlock> lock(scopeMapSpinLock);
        
        const std::uint32_t hash = static_cast<std::uint32_t>(IYF_THREAD_PROFILER_HASH(std::string(identifier)));
        const ScopeKey scopeKey(hash);
        
        auto result = scopes.find(scopeKey);
        if (result != scopes.end()) {
            return result->second;
        }
        
        ScopeInfo scopeInfo(scopeKey, scopeName, functionName, fileName, line, tag);
        auto insertionResult = scopes.emplace(scopeKey, std::move(scopeInfo));
        
        return insertionResult.first->second;
    }
    
    /// \brief Inserts the start of the scope.
    /// 
    /// \param key The key of the scope.
    inline void insertScopeStart(ScopeKey key) {
        const std::size_t threadID = GetCurrentThreadID();
        ThreadData& threadData = threads[threadID];
        
        threadData.depth++;
        
        if (isRecording()) {
            threadData.activeStack.emplace_back(key, threadData.depth, ProfilerClock::now().time_since_epoch());
        } else {
            // Don't even call the clock
            threadData.activeStack.emplace_back(key, threadData.depth, ProfilerClock::time_point().time_since_epoch());
        }
        
#ifdef IYF_PROFILER_WITH_COOKIE
        threadData.activeStack.back().setCookie(threadData.cookie);
        threadData.cookie++;
#endif // IYF_PROFILER_WITH_COOKIE
    }
    
    /// \brief Inserts the end of the scope.
    /// 
    /// \param key The key of the scope.
    inline void insertScopeEnd(ScopeKey key) {
        const std::size_t threadID = GetCurrentThreadID();
        ThreadData& threadData = threads[threadID];
        
        if (isRecording()) {
            auto& lastElement = threadData.activeStack.back();
            
            IYF_ASSERT(key == lastElement.getKey());
            
            if (lastElement.isValid()) {
                lastElement.setEnd(ProfilerClock::now().time_since_epoch());
                
                std::lock_guard<Spinlock> lock(threadData.recordSpinLock);
                threadData.recordedEvents.emplace_back(std::move(lastElement));
            }
        }
        
        threadData.activeStack.pop_back();
        
        threadData.depth--;
    }
    
    /// \brief Enables or disables recording.
    inline void setRecording(bool state) {
        recording.store(state, std::memory_order_release);
    }
    
    /// \brief Checks if the ThreadProfiler is recording or not.
    ///
    /// \return If the ThreadProfiler is recording or not.
    inline bool isRecording() const {
        return recording.load(std::memory_order_acquire);
    }
    
    /// \brief Starts the next frame.
    inline void nextFrame() {
        std::lock_guard<Spinlock> lock(frameSpinLock);
        
        const std::uint64_t lastFrameNumber = frameNumber;
        frameNumber++;

        auto now = ProfilerClock::now().time_since_epoch();
        
        if (!frames.empty()) {
            FrameData& lastFrame = frames.back();
            
            if (lastFrame.getNumber() == lastFrameNumber) {
                lastFrame.setEnd(now);
            }
        }
        
        if (isRecording()) {
            frames.emplace_back(frameNumber, now);
        }
    }
    
    /// \brief Obtains the current results and clears the current data buffers.
    ///
    /// \return All currently recorded data.
    ProfilerResults getResults();
private:
    /// Internal struct used to manage per-thread data
    struct ThreadData {
#ifdef IYF_PROFILER_WITH_COOKIE
        ThreadData() : depth(-1), cookie(0) {
            activeStack.reserve(256);
        }
#else // IYF_PROFILER_WITH_COOKIE
        ThreadData() : depth(-1) {
            activeStack.reserve(256);
        }
#endif // IYF_PROFILER_WITH_COOKIE
        /// Sadly, even if profiling is disabled, I need to keep track of the stack
        /// state. The impact is minimized by using a vector with plenty of reserved
        /// space instead of a deque (deques may deallocate storage when shrinking).
        /// Moreover, I don't even call now() and keep the start and end times empty.
        std::vector<RecordedEvent> activeStack;
        
        /// Protects the records
        Spinlock recordSpinLock;
        
        /// Using a queue here to store events because it is faster than a list for
        /// our purposes.
        std::deque<RecordedEvent> recordedEvents;
        
        /// Current stack depth.
        std::int32_t depth;
    #ifdef IYF_PROFILER_WITH_COOKIE
        std::uint64_t cookie;
    #endif // IYF_PROFILER_WITH_COOKIE
    };
    
    /// Used to tell the threads if the profiler is currently recording or not.
    std::atomic<bool> recording;
    
    /// Used to protect the scope map.
    Spinlock scopeMapSpinLock;
    
    /// Contains information on all scopes tracked by the ThreadProfiler. This is used
    /// to avoid storing tons of duplicate data every time we start profiling a scope.
    std::unordered_map<ScopeKey, ScopeInfo> scopes;
    
    /// Contains per-thread data
    std::array<ThreadData, IYF_THREAD_PROFILER_MAX_THREAD_COUNT> threads;
    
    std::uint64_t frameNumber;
    Spinlock frameSpinLock;
    std::deque<FrameData> frames;
};

/// Returns a reference to the default ThreadProfiler instance
ThreadProfiler& GetThreadProfiler();

/// \brief A class that helps with automatic scope start and end tracking.
class ScopeProfilerHelper {
public:
    /// \brief Starts tracking the scope.
    ScopeProfilerHelper(ScopeKey scopeKey) : scopeKey(scopeKey) {
        GetThreadProfiler().insertScopeStart(scopeKey);
    }
    
    /// \brief Finishes tracking the scope.
    ~ScopeProfilerHelper() {
        GetThreadProfiler().insertScopeEnd(scopeKey);
    }
private:
    ScopeKey scopeKey;
};

/// \brief Contains results that were recorded by the ThreadProfiler.
class ProfilerResults {
public:
    /// \brief A record of a tag name and color.
    ///
    /// We need to store this because the end users will use different thread 
    /// profiler settings.
    class TagNameAndColor {
    public:
        /// \brief Constructs an empty TagNameAndColor object.
        TagNameAndColor() 
            : name(GetTagName(ProfilerTag::NoTag)), color(GetTagColor(ProfilerTag::NoTag)) {}
        
        /// \brief Constructs a TagNameAndColor object using explicit values.
        TagNameAndColor(std::string name, ScopeColor color) : name(name), color(color) {}

        /// Returns the name of the tag.
        ///
        /// \return The name of the tag.
        inline const std::string& getName() const {
            return name;
        }
        
        /// Returns the color of the tag.
        ///
        /// \return The color of the tag.
        inline const ScopeColor& getColor() const {
            return color;
        }
        
        /// \brief A comparison operator.
        inline friend bool operator==(const TagNameAndColor& a, const TagNameAndColor& b) {
            return (a.name == b.name) && (a.color == b.color);
        }
    private:
        /// \brief The name of the tag.
        std::string name;
        
        /// \brief The color of the tag.
        ScopeColor color;
    };
    
    /// \brief A comparison operator.
    inline friend bool operator==(const ProfilerResults& a, const ProfilerResults& b) {
        return (a.frames == b.frames) &&
               (a.scopes == b.scopes) &&
               (a.tags == b.tags) &&
               (a.events == b.events) &&
               (a.threadNames == b.threadNames) &&
               (a.frameDataMissing == b.frameDataMissing) &&
               (a.anyRecords == b.anyRecords) &&
               (a.withCookie == b.withCookie);
    }
    
    /// \brief Creates a ProfilerResults instance by reading data from a file.
    ///
    /// \warning This reads the data in native byte order.
    ///
    /// \return A ProfilerResults instance or a nullopt if file reading failed.
    static std::optional<ProfilerResults> LoadFromFile(const std::string& path);
    
    /// \brief Writes the data to a file.
    ///
    /// \warning This writes the data in native byte order.
    ///
    /// \return true if the operation succeeded, false if it didn't (e.g., if you 
    /// didn't have the permission to write to the specified file).
    bool writeToFile(const std::string& path) const;
    
    /// \brief Writes the data to a human readable std::string.
    ///
    /// \return A string that contains human readable data.
    std::string writeToString() const;
    
    /// \brief Access the FrameData container.
    const std::deque<FrameData>& getFrames() const {
        return frames;
    }
    
    /// \brief Access the RecordedEvent container.
    const std::deque<RecordedEvent>& getEvents(std::size_t threadID) const {
        return events[threadID];
    }
    
    /// \brief Access the ScopeInfo container.
    const std::unordered_map<ScopeKey, ScopeInfo>& getScopes() const {
        return scopes;
    }
    
    /// \brief Access the tag data container.
    const std::unordered_map<std::uint32_t, TagNameAndColor> getTags() const {
        return tags;
    }
    
    /// \brief Checks if frame data is missing.
    ///
    /// If this is true (may happen if the profiler didn't run for the whole frame or
    /// if you didn't call IYF_PROFILER_NEXT_FRAME), the frame deque will contain a 
    /// single artificial frame that starts together with the earliest recorded event
    /// and ends together with the last recorded event. If hasAnyRecords() is false
    /// as well, the start and the end will be equal to 0 and 1.
    bool isFrameDataMissing() const {
        return frameDataMissing;
    }
    
    /// \brief Checks if this ProfilerResults contains any records.
    ///
    /// If this is false, no events were recorded and the event deques will be empty.
    /// This may happen if you forgot to call IYF_PROFILER_SET_RECORDING(true), 
    /// disabled the profiler before it could record any data or if you didn't 
    /// instrument your code with IYF_PROFILE calls. Depending on the cause, the scope
    /// data may still be available.
    bool hasAnyRecords() const {
        return anyRecords;
    }
private:
    friend class ThreadProfiler;
    
    /// The default constructed state isn't valid. It needs to be processed by the
    /// ThreadProfiler.
    ProfilerResults() 
        : frameDataMissing(false), anyRecords(false), withCookie(false) {}
    
    std::deque<FrameData> frames;
    std::unordered_map<ScopeKey, ScopeInfo> scopes;
    std::unordered_map<std::uint32_t, TagNameAndColor> tags;
    std::vector<std::deque<RecordedEvent>> events;
    std::vector<std::string> threadNames;
    bool frameDataMissing;
    bool anyRecords;
    bool withCookie;
}; 

}

/// \brief Stringify the parameter.
///
/// \param a Object to stringify.
#define IYF_STRINGIFY(a) #a

/// \brief Used to expand and stringify a macro.
///
/// \param a Object to expand and stringify.
#define IYF_EXPAND_STRINGIFY(a) IYF_STRINGIFY(a)

/// \brief Used to expand a macro.
///
/// \param a Object to expand
#define IYF_EXPAND(a) a

/// \brief Concatenates two strings.
///
/// \param a The left side of the final string.
/// \param b The right side of the final string.
#define IYF_CONCAT(a, b) a ## b

/// \brief Profiles a named scope with a tag.
#define IYF_PROFILE_2(name, tag) \
static iyft::ScopeInfo& ScopeInfo##name = iyft::GetThreadProfiler().insertScopeInfo(\
    #name,\
    __FILE__ ":" IYF_EXPAND_STRINGIFY(__LINE__),\
    __func__,\
    __FILE__,\
    __LINE__,\
    tag); \
    iyft::ScopeProfilerHelper ProfiledScope##name(ScopeInfo##name.getKey());

/// \brief Profiles a named scope without a tag.
#define IYF_PROFILE_1(name) \
static iyft::ScopeInfo& ScopeInfo##name = iyft::GetThreadProfiler().insertScopeInfo(\
    #name,\
    __FILE__ ":" IYF_EXPAND_STRINGIFY(__LINE__),\
    __func__,\
    __FILE__,\
    __LINE__,\
    iyft::ProfilerTag::NoTag); \
    iyft::ScopeProfilerHelper ProfiledScope##name(ScopeInfo##name.getKey());

/// \brief Concats the name with an ID to choose an appropriate macro.
#define IYF_SELECT(NAME, ID) IYF_CONCAT(NAME##_, ID)

/// \brief Together with IYF_VA_SIZE helps with macro "overloading".
/// 
/// \remark Based on https://stackoverflow.com/questions/16683146/can-macros-be-overloaded-by-number-of-arguments
/// and https://stackoverflow.com/a/11172679
#define IYF_GET_COUNT(_1, _2, COUNT, ...) COUNT

/// \brief Together with IYF_GET_COUNT helps with macro "overloading".
/// 
/// \remark Based on https://stackoverflow.com/questions/16683146/can-macros-be-overloaded-by-number-of-arguments
/// and https://stackoverflow.com/a/11172679
#define IYF_VA_SIZE(...) IYF_EXPAND(IYF_GET_COUNT(__VA_ARGS__, 2, 1, discardMe))

/// \brief A helper that chooses one of several functions.
#define IYF_PROFILE_MACRO_PICKER(NAME, ...) IYF_SELECT(NAME, IYF_VA_SIZE(__VA_ARGS__))(__VA_ARGS__)

/// \brief Depending on the number of parameters, chooses one of profiling macros.
#define IYF_PROFILE(...) IYF_PROFILE_MACRO_PICKER(IYF_PROFILE, __VA_ARGS__)

/// \brief Used to start or stop a recording
///
/// \param a A boolean. If true, starts a recording, if false, stops it.
#define IYF_PROFILER_SET_RECORDING(a) iyft::GetThreadProfiler().setRecording(a);

/// \brief Starts the next frame.
#define IYF_PROFILER_NEXT_FRAME iyft::GetThreadProfiler().nextFrame();

/// \brief Returns the status of the profiler as a ProfilerStatus value. 
#define IYF_PROFILER_STATUS (iyft::GetThreadProfiler().isRecording() ? iyft::ProfilerStatus::EnabledAndRecording : iyft::ProfilerStatus::EnabledAndNotRecording)

/// \brief Stops the recording (if it's running) and writes the data to a string.
#define IYF_PROFILER_RESULT_STRING iyft::GetThreadProfiler().getResults().writeToString()

/// \brief Writes results to a file.
///
/// \param fileName The name of the destination file.
///
/// \return true if the operation succeeded
#define IYF_PROFILER_RESULTS_TO_FILE(fileName) iyft::GetThreadProfiler().getResults().writeToFile(fileName);

#else //  defined(IYF_ENABLE_PROFILING)

// These will get optimized away if profiling is disabled
#define IYF_PROFILE(...) ((void)0);
#define IYF_PROFILER_SET_RECORDING(a) ((void)0);
#define IYF_PROFILER_NEXT_FRAME ((void)0);
#define IYF_PROFILER_STATUS iyft::ProfilerStatus::Disabled
#define IYF_PROFILER_RESULT_STRING std::string("PROFILER-IS-DISABLED")
#define IYF_PROFILER_RESULTS_TO_FILE(fileName) ((void)0);

#endif //  defined(IYF_ENABLE_PROFILING)

#endif // IYF_THREAD_PROFILER_HPP

#if defined IYF_THREAD_PROFILER_IMPLEMENTATION && !defined IYF_THREAD_PROFILER_IMPLEMENTATION_INCLUDED
#define IYF_THREAD_PROFILER_IMPLEMENTATION_INCLUDED

#include <chrono>
#include <mutex>
#include <stdexcept>
#include <cstring>
#include <thread>

using namespace std::chrono_literals;

namespace iyft {

class ThreadIDAssigner {
public:
    ThreadIDAssigner() : counter(0) {
        for (std::size_t i = 0; i < IYF_THREAD_PROFILER_MAX_THREAD_COUNT; ++i) {
            names[i] = std::string("Thread") + std::to_string(i);
        }
    }
    
    void assignNext(const char* name);
    
    inline std::size_t getThreadCount() const {
        std::lock_guard<std::mutex> lock(mutex);
        return counter;
    }
    
    /// This can be used to retrieve a name for a specific thread.
    ///
    /// \param id The id of the thread. Must be less than getThreadCount().
    inline std::string getThreadName(std::size_t id) const {
        std::lock_guard<std::mutex> lock(mutex);
        return names[id];
    }
private:
    mutable std::mutex mutex;
    std::size_t counter;
    std::array<std::string, IYF_THREAD_PROFILER_MAX_THREAD_COUNT> names;
};

static ThreadIDAssigner ThreadIDAssigner;

const std::size_t emptyID = static_cast<std::size_t>(-1);
static thread_local std::size_t CurrentThreadID = emptyID;
static thread_local std::string CurrentThreadName = "";

void ThreadIDAssigner::assignNext(const char* name) {
    std::lock_guard<std::mutex> lock(mutex);
    
    CurrentThreadID = counter;
    
    if (CurrentThreadID >= IYF_THREAD_PROFILER_MAX_THREAD_COUNT) {
        throw std::runtime_error("You've created more threads than allowed.");
    }
    counter++;
    
    if (name == nullptr || (std::strlen(name) == 0)) {
        names[CurrentThreadID] = std::string("Thread") + std::to_string(CurrentThreadID);
    } else {
        names[CurrentThreadID] = name;
    }
    
    CurrentThreadName = names[CurrentThreadID];
}

std::size_t GetCurrentThreadID() {
    if (CurrentThreadID == emptyID) {
        ThreadIDAssigner.assignNext(nullptr);
    }
    
    return CurrentThreadID;
}

const char* GetCurrentThreadName() {
    if (CurrentThreadID == emptyID) {
        ThreadIDAssigner.assignNext(nullptr);
    }
    
    return CurrentThreadName.c_str();
}

std::size_t GetRegisteredThreadCount() {
    return ThreadIDAssigner.getThreadCount();
}

bool AssignThreadName(const char* name) {
    if (CurrentThreadID == emptyID) {
        ThreadIDAssigner.assignNext(name);
        return true;
    } else {
        return false;
    }
}

}

#ifdef IYF_ENABLE_PROFILING
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace iyft {
ThreadProfiler& GetThreadProfiler() {
    static ThreadProfiler profiler;
    return profiler;
}

ProfilerResults ThreadProfiler::getResults() {
    setRecording(false);
    
    ProfilerResults results;
    
    {
        // Lock to make sure the scope map and the frame deque don't get enabled
        // while we're extracting their data
        std::lock_guard<Spinlock> scopeLock(scopeMapSpinLock);
        std::lock_guard<Spinlock> frameLock(frameSpinLock);
        
        results.frames.swap(frames);
        results.scopes = scopes;
        
        const std::size_t threadCount = GetRegisteredThreadCount();
        results.events.resize(threadCount);
        results.threadNames.resize(threadCount);
        
        for (std::size_t i = 0; i < threadCount; ++i) {
            ThreadData& threadData = threads[i];
            
            std::lock_guard<Spinlock> lock(threadData.recordSpinLock);
            results.events[i].swap(threadData.recordedEvents);
            results.threadNames[i] = ThreadIDAssigner.getThreadName(i);
        }
    }
    
    const std::uint32_t tagStart = static_cast<std::uint32_t>(ProfilerTag::NoTag);
    const std::uint32_t tagEnd = static_cast<std::uint32_t>(ProfilerTag::COUNT);
    for (std::uint32_t i = tagStart; i < tagEnd; ++i) {
        const ProfilerTag tag = static_cast<ProfilerTag>(i);
        ProfilerResults::TagNameAndColor nameAndColor(GetTagName(tag), GetTagColor(tag));
        
        results.tags.emplace(i, std::move(nameAndColor));
    }
    
    bool hasAnyRecords = false;
    
    for (const auto& t : results.events) {
        if (!t.empty()) {
            hasAnyRecords = true;
        }
    }
    
    results.anyRecords = hasAnyRecords;
    
    const std::size_t frameCount = results.frames.size();
    if (frameCount == 0 && !hasAnyRecords) {
        FrameData frame(0, std::chrono::nanoseconds(0));
        frame.setEnd(std::chrono::nanoseconds(1));
        results.frames.emplace_back(std::move(frame));
        
        results.frameDataMissing = true;
    } else if (frameCount == 0) {
        std::chrono::nanoseconds first(std::chrono::nanoseconds::max());
        std::chrono::nanoseconds last(std::chrono::nanoseconds::min());
        
        for (const auto& t : results.events) {
            if (!t.empty()) {
                first = std::min(first, t.front().getStart());
                last = std::max(last, t.back().getStart());
            }
        }
        
        IYF_ASSERT(first != std::chrono::nanoseconds::max());
        IYF_ASSERT(last != std::chrono::nanoseconds::min());
        
        FrameData frame(0, first);
        frame.setEnd(last);
        results.frames.emplace_back(std::move(frame));
        
        results.frameDataMissing = true;
    } else {
        FrameData& lastFrame = results.frames.back();
        
        if (!lastFrame.isComplete()) {
            lastFrame.setEnd(ProfilerClock::now().time_since_epoch());
        }
        
        results.frameDataMissing = false;
    }
    
#ifdef IYF_PROFILER_WITH_COOKIE
    results.withCookie = true;
#else // IYF_PROFILER_WITH_COOKIE
    results.withCookie = false;
#endif // IYF_PROFILER_WITH_COOKIE
    
    // Yeah, prettier and faster ways exist, but this is good enough
    for (auto& t : results.events) {
        if (t.size() > 1) {
            std::sort(t.begin(), t.end());
        }
    }
    
    return results;
}

inline static std::uint64_t ReadUInt64(std::istream& is) {
    std::uint64_t num;
    is.read(reinterpret_cast<char*>(&num), sizeof(std::uint64_t));
    return num;
}

inline static std::uint32_t ReadUInt32(std::istream& is) {
    std::uint32_t num;
    is.read(reinterpret_cast<char*>(&num), sizeof(std::uint32_t));
    return num;
}

inline static std::int32_t ReadInt32(std::istream& is) {
    std::int32_t num;
    is.read(reinterpret_cast<char*>(&num), sizeof(std::int32_t));
    return num;
}

inline static std::uint8_t ReadUInt8(std::istream& is) {
    std::uint8_t num;
    is.read(reinterpret_cast<char*>(&num), sizeof(std::uint8_t));
    return num;
}

inline static std::chrono::nanoseconds ReadNanos(std::istream& is) {
    std::int64_t num;
    is.read(reinterpret_cast<char*>(&num), sizeof(std::int64_t));
    return std::chrono::nanoseconds(num);
}

inline static std::string ReadString(std::istream& is) {
    std::uint16_t length;
    is.read(reinterpret_cast<char*>(&length), sizeof(std::uint16_t));
    
    std::string string(length, '\0');
    is.read(&string[0], length);
    
    return string;
}

std::optional<ProfilerResults> ProfilerResults::LoadFromFile(const std::string& path) {
    // Warn about a file with cookies when no cookie support is enabled
    ProfilerResults pr;
    
    std::ifstream is(path, std::ios::binary);
    
    if (!is.is_open()) {
        return std::nullopt;
    }
    
    // Magic number
    if (is.get() != 'I' || is.get() != 'Y' || is.get() != 'F' || is.get() != 'R') {
        return std::nullopt;
    }
    
    // Version and some parameters
    if (is.get() != 1) {
        return std::nullopt;
    }
    
    pr.frameDataMissing = is.get();
    pr.anyRecords = is.get();
    pr.withCookie = is.get();
    
    // Thread names
    const std::uint64_t threadCount = ReadUInt64(is);
    pr.threadNames.reserve(threadCount);
    
    for (std::uint64_t i = 0; i < threadCount; ++i) {
        pr.threadNames.push_back(ReadString(is));
    }

    // Frames
    const std::uint64_t frameCount = ReadUInt64(is);
    
    for (std::uint64_t i = 0; i < frameCount; ++i) {
        std::uint64_t frameNumber = ReadUInt64(is);
        std::chrono::nanoseconds start = ReadNanos(is);
        std::chrono::nanoseconds end = ReadNanos(is);
        
        FrameData frame(frameNumber, start);
        frame.setEnd(end);
        
        pr.frames.emplace_back(std::move(frame));
    }
    
    // Tags
    const std::uint64_t tagCount = ReadUInt64(is);
    pr.tags.reserve(tagCount);
    
    for (std::uint64_t i = 0; i < tagCount; ++i) {
        std::uint32_t tagID = ReadUInt32(is);
        IYF_ASSERT(i == tagID);
        
        std::string name = ReadString(is);
        
        
        const std::uint8_t r = ReadUInt8(is);
        const std::uint8_t g = ReadUInt8(is);
        const std::uint8_t b = ReadUInt8(is);
        const std::uint8_t a = ReadUInt8(is);
        ScopeColor color(r, g, b, a);
        
        TagNameAndColor nameAndColor(std::move(name), std::move(color));
        
        pr.tags.emplace(tagID, std::move(nameAndColor));
    }
    
    // Scope info
    const std::uint64_t scopeCount = ReadUInt64(is);
    pr.scopes.reserve(scopeCount);
    
    for (std::uint64_t i = 0; i < scopeCount; ++i) {
        const ScopeKey key(ReadUInt32(is));
        
        const ProfilerTag tag = static_cast<ProfilerTag>(ReadUInt32(is));
        const std::string name = ReadString(is);
        const std::string functionName = ReadString(is);
        const std::string fileName = ReadString(is);
        const std::uint32_t lineNumber = ReadUInt32(is);
        
        ScopeInfo scopeInfo(key, name, functionName, fileName, lineNumber, tag);
        pr.scopes.emplace(key, std::move(scopeInfo));
    }
    
    // Events for each thread
    pr.events.resize(threadCount);
    for (std::uint64_t i = 0; i < threadCount; ++i) {
        const std::uint64_t eventCount = ReadUInt64(is);
        
        std::deque<RecordedEvent>& threadData = pr.events[i];
        for (std::uint64_t j = 0; j < eventCount; ++j) {
            const ScopeKey key(ReadUInt32(is));
            
            const std::int32_t depth = ReadInt32(is);
            const std::chrono::nanoseconds start = ReadNanos(is);
            const std::chrono::nanoseconds end = ReadNanos(is);
            
            RecordedEvent event(key, depth, start);
            event.setEnd(end);
#ifdef IYF_PROFILER_WITH_COOKIE
            const std::uint64_t cookie = 0;
            if (pr.withCookie) {
                cookie = ReadUInt64(is);
            }
            event.setCookie(cookie);
#else // IYF_PROFILER_WITH_COOKIE
            if (pr.withCookie) {
                ReadUInt64(is);
            }
#endif // IYF_PROFILER_WITH_COOKIE

            threadData.emplace_back(std::move(event));
        }
    }
    return pr;
}

inline static void WriteUInt64(std::ostream& os, std::uint64_t num) {
    os.write(reinterpret_cast<const char*>(&num), sizeof(std::uint64_t));
}

inline static void WriteUInt32(std::ostream& os, std::uint32_t num) {
    os.write(reinterpret_cast<const char*>(&num), sizeof(std::uint32_t));
}

inline static void WriteInt32(std::ostream& os, std::int32_t num) {
    os.write(reinterpret_cast<const char*>(&num), sizeof(std::int32_t));
}

inline static void WriteUInt8(std::ostream& os, std::uint8_t num) {
    os.write(reinterpret_cast<const char*>(&num), sizeof(std::uint8_t));
}

inline static void WriteNanos(std::ostream& os, std::chrono::nanoseconds ns) {
    const std::int64_t time = ns.count();
    os.write(reinterpret_cast<const char*>(&time), sizeof(std::int64_t));
}

inline static void WriteString(std::ostream& os, const std::string& string) {
    const std::uint16_t length = static_cast<std::uint16_t>(string.length());
    os.write(reinterpret_cast<const char*>(&length), sizeof(std::uint16_t));
    os.write(string.c_str(), length);
}

bool ProfilerResults::writeToFile(const std::string& path) const {
    std::ofstream os(path, std::ios::binary);
    
    if (!os.is_open()) {
        return false;
    }
    
    // Magic number
    os.put('I');
    os.put('Y');
    os.put('F');
    os.put('R'); // P is used by different files in the IYFEngine
    
    // Version and some parameters
    os.put(1); // Version [1 byte]
    os.put(frameDataMissing);
    os.put(anyRecords);
    os.put(withCookie);
    
    IYF_ASSERT(threadNames.size() == events.size());
    
    // Thread names
    WriteUInt64(os, threadNames.size());
    for (const std::string& name : threadNames) {
        WriteString(os, name);
    }

    // Frames
    WriteUInt64(os, frames.size());
    for (const FrameData& frame : frames) {
        WriteUInt64(os, frame.getNumber());
        WriteNanos(os, frame.getStart());
        WriteNanos(os, frame.getEnd());
    }
    
    // Tags
    WriteUInt64(os, tags.size());
    for (const auto& t : tags) {
        WriteUInt32(os, t.first);
        
        WriteString(os, t.second.getName());
        
        const ScopeColor& c = t.second.getColor();
        WriteUInt8(os, c.getRed());
        WriteUInt8(os, c.getGreen());
        WriteUInt8(os, c.getBlue());
        WriteUInt8(os, c.getAlpha());
    }
    
    // Scope info
    WriteUInt64(os, scopes.size());
    for (const auto& s : scopes) {
        const auto& scope = s.second;
        
        WriteUInt32(os, scope.getKey().getValue());
        WriteUInt32(os, static_cast<std::uint32_t>(scope.getTag()));
        WriteString(os, scope.getName());
        WriteString(os, scope.getFunctionName());
        WriteString(os, scope.getFileName());
        WriteUInt32(os, scope.getLineNumber());
    }
    
    // Events for each thread (the number of threads is already known, no need to
    // write it)
    for (const std::deque<RecordedEvent>& threadEvents : events) {
        WriteUInt64(os, threadEvents.size());
        for (const RecordedEvent& e : threadEvents) {
            WriteUInt32(os, e.getKey().getValue());
            WriteInt32(os, e.getDepth());
            WriteNanos(os, e.getStart());
            WriteNanos(os, e.getEnd());
#ifdef IYF_PROFILER_WITH_COOKIE
            WriteUInt64(e.getCookie());
#endif // IYF_PROFILER_WITH_COOKIE
        }
    }
    
    return true;
}

static void writeFrameData(std::stringstream& ss, const FrameData& frame) {
    const IYF_THREAD_TEXT_OUTPUT_DURATION duration = frame.getDuration();
    ss << "  FRAME: " << frame.getNumber() 
       << "; Duration: " << duration.count() << IYF_THREAD_TEXT_OUTPUT_NAME "\n";
}

std::string ProfilerResults::writeToString() const {
    std::stringstream ss;
    
    IYF_ASSERT(frames.size() > 0);
    
    for (std::size_t i = 0; i < threadNames.size(); ++i) {
        const std::deque<RecordedEvent>& data = events[i];
        
        ss << "THREAD: " << threadNames[i] << "; Event count: " << data.size() << "\n";
        
        auto frameIter = frames.begin();
        const auto lastFrame = std::prev(frames.end());
        
        writeFrameData(ss, *frameIter);
        for (const auto& e : data) {
            if (e.getStart() < (*frameIter).getStart()) {
                // This event happened before the first recorded frame. Skip it.
                ss << "Skiped early event\n";
                continue;
            } else if ((e.getStart() > (*frameIter).getEnd()) && frameIter == lastFrame) {
                // This event happened after the last recorded frame. Skip it.
                ss << "Skipped late event\n";
                continue;
            } else if ((e.getStart() > (*frameIter).getEnd())) {
                while ((e.getStart() > (*frameIter).getEnd()) && frameIter != lastFrame) {
                    frameIter++;
                }
                
                writeFrameData(ss, *frameIter);
            }
            
            const auto result = scopes.find(e.getKey());
            IYF_ASSERT(result != scopes.end());
            
            const ScopeInfo& info = result->second;
            
            const IYF_THREAD_TEXT_OUTPUT_DURATION d = e.getDuration();
            
            const std::size_t offset = static_cast<std::size_t>(e.getDepth() * 2 + 4);
            const std::string depthOffset(offset, ' ');
            ss << depthOffset << "SCOPE: " << info.getName()
#ifdef IYF_PROFILER_WITH_COOKIE
               << "; Cookie: " << e.getCookie()
#endif // IYF_PROFILER_WITH_COOKIE
               << "; Function: " << info.getFunctionName()
               << "; Duration: " << d.count() << IYF_THREAD_TEXT_OUTPUT_NAME "\n";
        }
    }
    
    return ss.str();
}
}
#endif // IYF_ENABLE_PROFILING

#endif // defined IYF_THREAD_PROFILER_IMPLEMENTATION && !defined IYF_THREAD_PROFILER_IMPLEMENTATION_INCLUDED

