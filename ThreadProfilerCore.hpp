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

/// \file ThreadProfilerCore.hpp Contains all classes and macros used for profiling. This
/// header should only be included in the files where you access results

#ifndef IYFT_THREAD_PROFILER_CORE_HPP
#define IYFT_THREAD_PROFILER_CORE_HPP

#include <mutex>
#include <unordered_map>
#include <deque>
#include <vector>
#include <chrono>
#include <array>
#include <string>
#include <memory>
#include "Spinlock.hpp"

#ifdef IYFT_PROFILER_WITH_IMGUI
#include "imgui.h"
#include <cinttypes>
#include <cmath>
#endif // IYFT_PROFILER_WITH_IMGUI

namespace iyft {    
/// \brief The clock that the profiler will use.
using ProfilerClock = std::chrono::high_resolution_clock;

#ifdef IYFT_PROFILER_WITH_COOKIE
using ProfilerCookie = std::uint64_t;
#endif // IYFT_PROFILER_WITH_COOKIE

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

#ifdef NDEBUG
#define IYFT_ASSERT(a) ((void)(a))
#else // NDEBUG
#include <cassert>
/// A modified assert macro that doesn't cause unused variable warnings when asserions are
/// disabled.
#define IYFT_ASSERT(a) assert(a);
#endif // NDEBUG


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
        : start(start), end() {}
    
    virtual ~TimedProfilerObject() = default;
    
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
    
#ifdef IYFT_PROFILER_WITH_COOKIE
    inline ProfilerCookie getCookie() const {
        return cookie;
    }
    
    inline void setCookie(ProfilerCookie cookie) {
        this->cookie = cookie;
    }
#endif // IYFT_PROFILER_WITH_COOKIE
    
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
#ifdef IYFT_PROFILER_WITH_COOKIE
               (a.cookie == b.cookie) &&
#endif // IYFT_PROFILER_WITH_COOKIE
               (a.getStart() == b.getStart()) &&
               (a.getEnd() == a.getEnd());
    }
private:
    ScopeKey key;
    std::int32_t depth;
#ifdef IYFT_PROFILER_WITH_COOKIE
    ProfilerCookie cookie;
#endif // IYFT_PROFILER_WITH_COOKIE
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
    
    /// \brief A comparison operator.
    inline friend bool operator<(const FrameData& a, const FrameData& b) {
        return a.getStart() < b.getStart();
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
    ThreadProfiler() : recording(false), frameNumber(0) { }
    
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
        
        const std::uint32_t hash = static_cast<std::uint32_t>(IYFT_THREAD_PROFILER_HASH(std::string(identifier)));
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
    /// \param info A ScopeInfo instance.
    inline void insertScopeStart(const ScopeInfo& info) {
        const ScopeKey key = info.getKey();
        
        const std::size_t threadID = GetCurrentThreadID();
        ThreadData& threadData = threads[threadID];
        
        threadData.depth++;
        
//         if (isRecording()) {
            threadData.activeStack.emplace_back(key, threadData.depth, ProfilerClock::now().time_since_epoch());
//         } else {
//             // Don't even call the clock
//             threadData.activeStack.emplace_back(key, threadData.depth, ProfilerClock::time_point().time_since_epoch());
//         }
//         
#ifdef IYFT_PROFILER_WITH_COOKIE
        threadData.activeStack.back().setCookie(threadData.cookie);
        threadData.cookie++;
#endif // IYFT_PROFILER_WITH_COOKIE
    }
    
    /// \brief Inserts the end of the scope.
    /// 
    /// \param info A ScopeInfo instance.
    inline void insertScopeEnd(const ScopeInfo& info) {
        const ScopeKey key = info.getKey();
        
        const std::size_t threadID = GetCurrentThreadID();
        ThreadData& threadData = threads[threadID];
        
        if (isRecording()) {
            auto& lastElement = threadData.activeStack.back();
            
            IYFT_ASSERT(key == lastElement.getKey());
            
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
#ifdef IYFT_PROFILER_WITH_COOKIE
        ThreadData() : depth(-1), cookie(0) {
            activeStack.reserve(256);
        }
#else // IYFT_PROFILER_WITH_COOKIE
        ThreadData() : depth(-1) {
            activeStack.reserve(256);
        }
#endif // IYFT_PROFILER_WITH_COOKIE
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
    #ifdef IYFT_PROFILER_WITH_COOKIE
        std::uint64_t cookie;
    #endif // IYFT_PROFILER_WITH_COOKIE
    };
    
    /// Used to tell the threads if the profiler is currently recording or not.
    std::atomic<bool> recording;
    
    /// Used to protect the scope map.
    Spinlock scopeMapSpinLock;
    
    /// Contains information on all scopes tracked by the ThreadProfiler. This is used
    /// to avoid storing tons of duplicate data every time we start profiling a scope.
    std::unordered_map<ScopeKey, ScopeInfo> scopes;
    
    /// Contains per-thread data
    std::array<ThreadData, IYFT_THREAD_PROFILER_MAX_THREAD_COUNT> threads;
    
    std::uint64_t frameNumber;
    Spinlock frameSpinLock;
    std::deque<FrameData> frames;
};

/// Returns a reference to the default ThreadProfiler instance
ThreadProfiler& GetThreadProfiler();

/// \brief A minimal interval tree used for windowing queries.
///
/// This is based on https://www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf
/// and https://en.wikipedia.org/wiki/Interval_tree#Augmented_tree
template <typename T>
class InsertOnlyIntervalTree {
public:
    /// \brief The type used for interval values, e.g., float, std::chrono::nanoseconds, etc.
    using IntervalType = typename T::IntervalType;
    
    /// \brief Creates a new InsertOnlyIntervalTree that can contain up to maxNodes
    ///
    /// \param maxNodes The maximum number of node this InsertOnlyIntervalTree can
    /// contain.
    InsertOnlyIntervalTree(std::size_t maxNodes) : root(nullptr), maxNodes(maxNodes) {
        nodes.reserve(maxNodes);
    }
    
    /// \brief Prints the tree nodes in pre order.
    std::string print() const;
    
    /// \brief Finds intervals that intersect the searchInterval.
    ///
    /// \param searchInterval The interval to intersect with.
    /// \param intervals Returns all intervals that intersect with the searchInterval
    void findIntervals(const T& searchInterval, std::vector<T>& intervals) const {
        findIntervals(root, searchInterval, intervals);
    }
    
    /// \brief Returns the number of intervals stored in the tree.
    std::size_t size() const {
        return nodes.size();
    }
    
    /// \brief Recalculates the maximums. Must be called after inserting all intervals.
    void updateMaximumValues() {
        updateMaximumValues(root);
    }
    
    /// \brief Inserts a new interval
    void insert(const T& item) {
        root = insert(root, item);
        root->isRed = false;
    }
private:
    struct Node {
        inline Node(T item) : left(nullptr), right(nullptr), sibling(nullptr),  item(item), maximum(), isRed(true) {}
        
        Node* left;
        Node* right;
        Node* sibling;
        T item;
        IntervalType maximum;
        bool isRed;
    };
    
    void findIntervals(const Node* node, const T& searchInterval, std::vector<T>& intervals) const {
        if (node == nullptr) {
            return;
        }
        
        if (searchInterval.getStart() > node->maximum) {
            return;
        }
        
        if (node->left != nullptr) {
            findIntervals(node->left, searchInterval, intervals);
        }
        
        const Node* currentNode = node;
        while (currentNode != nullptr) {
            if (overlapsWith(currentNode->item, searchInterval)) {
                intervals.push_back(currentNode->item);
            }
            
            currentNode = currentNode->sibling;
        }
        
        if (searchInterval.getEnd() < node->item.getStart()) {
            return;
        }
        
        if (node->right != nullptr) {
            findIntervals(node->right, searchInterval, intervals);
        }
    }
    
    inline bool overlapsWith(const T& a, const T& b) const {
        return a.getStart() <= b.getEnd() && b.getStart() <= a.getEnd();
    }
    
    inline bool isRed(Node* h) const {
        if (h != nullptr) {
            return h->isRed;
        } else {
            return false;
        }
    }
    
    inline Node* insert(Node* h, const T& item) {
        if (h == nullptr) {
            return newNode(item);
        }
        
        if (isRed(h->left) && isRed(h->right)) {
            flipColors(h);
        }
        
        if (item == h->item) {
            Node* listItem = h;
            while (listItem->sibling != nullptr) {
                listItem = listItem->sibling;
            }
            
            listItem->sibling = newNode(item);
        } else if (item < h->item) {
            h->left = insert(h->left, item);
        } else {
            h->right = insert(h->right, item);
        }
        
        if (isRed(h->right) && !(isRed(h->left))) {
            h = rotateLeft(h);
        }
        
        if (isRed(h->left) && isRed(h->left->left)) {
            h = rotateRight(h);
        }
        
        return h;
    }
    
    inline Node* newNode(const T& item) {
        if (nodes.size() + 1 > maxNodes) {
            throw std::length_error("Can't insert more than maxNodes nodes");
        }
        
        nodes.emplace_back(item);
        return &nodes.back();
    }
    
    inline void flipColors(Node* h) {
        h->isRed = !h->isRed;
        h->left->isRed = !h->left->isRed;
        h->right->isRed = !h->right->isRed;
    }
    
    inline Node* rotateLeft(Node* h) {
        Node* temp = h->right;
        h->right = temp->left;
        temp->left = h;
        temp->isRed = h->isRed;
        h->isRed = true;
        return temp;
    }
    
    inline Node* rotateRight(Node* h) {
        Node* temp = h->left;
        h->left= temp->right;
        temp->right= h;
        temp->isRed = h->isRed;
        h->isRed = true;
        return temp;
    }
    
    void printTreePreOrder(std::stringstream& ss, const Node* node, std::size_t depth) const;
    
    inline IntervalType getMaximumSelf(const Node* node) const {
        IntervalType selfMax = node->item.getEnd();
        
        const Node* listItem = node->sibling;
        while (listItem != nullptr) {
            selfMax = std::max(selfMax, listItem->item.getEnd());
            
            listItem = listItem->sibling;
        }
        
        return selfMax;
    }
    
    IntervalType updateMaximumValues(Node* node) {
        if (node == nullptr) {
            return IntervalType();
        }
        
        const IntervalType self = getMaximumSelf(node);
        const IntervalType left = updateMaximumValues(node->left);
        const IntervalType right = updateMaximumValues(node->right);
        
        const IntervalType maximum = std::max(std::max(self, left), right);
        node->maximum = maximum;
        return maximum;
    }
    
    Node* root;
    std::size_t maxNodes;
    std::vector<Node> nodes;
};

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

/// \brief Possible results of the drawInImGui() function
enum class ProfilerDrawResult {
    DrawnSuccessfully, ///< Contents of this ProfilerResults object were drawn successfully
    DrawingFailed, ///< An error message was drawn
    ImGuiNotAvailable, ///< The profiler was built without IYFT_PROFILER_WITH_IMGUI and drawing is not available
};

#ifdef IYFT_PROFILER_WITH_IMGUI

class ProfilerResultDrawData {
public:
    ProfilerResultDrawData(const ProfilerResults* results);
    ProfilerDrawResult draw(float scale);
private:
    const ProfilerResults* results;
    
    struct FullEventData {
        using IntervalType = std::chrono::nanoseconds;
        
        FullEventData(std::size_t id, const TimedProfilerObject* event, const ScopeInfo* scope, const TagNameAndColor* nameAndColor)
            : id(id), event(event), scope(scope), nameAndColor(nameAndColor) {}
        
        inline friend bool operator<(const FullEventData& a, const FullEventData& b) {
            return a.event->getStart() < b.event->getStart();
        }
        
        inline friend bool operator==(const FullEventData& a, const FullEventData& b) {
            return a.event->getStart() == b.event->getStart();
        }
        
        void print(std::stringstream& ss) const;
        
        inline std::chrono::nanoseconds getStart() const {
            return event->getStart();
        }
        
        inline std::chrono::nanoseconds getEnd() const {
            return event->getEnd();
        }
        
        const std::size_t id;
        const TimedProfilerObject* event;
        const ScopeInfo* scope;
        const TagNameAndColor* nameAndColor;
    };
    
    enum class ValidationStatus {
        Validated,
        Invalid,
        Pending
    };
    
    struct ScopeStats {
        inline constexpr ScopeStats() 
            : totalCalls(0), averageCallDuration(), minCallDuration(), shortestFrameNumber(0), maxCallDuration(), longestFrameNumber(0) {}
        
        std::uint64_t totalCalls;
        std::chrono::duration<float, std::milli> averageCallDuration;
        
        std::chrono::duration<float, std::milli> minCallDuration;
        std::uint64_t shortestFrameNumber;
        
        std::chrono::duration<float, std::milli> maxCallDuration;
        std::uint64_t longestFrameNumber;
    };
    
    struct FullScopeData {
        FullScopeData(const ScopeInfo* scopeInfo, const TagNameAndColor* tagInfo, const ScopeStats* stats)
            : scopeInfo(scopeInfo), tagInfo(tagInfo), stats(stats) {}
        
        const ScopeInfo* scopeInfo;
        const TagNameAndColor* tagInfo;
        const ScopeStats* stats;
    };
    
    void drawScopeInfoColumnName(const char* name, std::vector<FullScopeData>& sortedScopes,
                            bool (*const ascendingSort)(const FullScopeData& a, const FullScopeData& b),
                            bool (*const descendingSort)(const FullScopeData& a, const FullScopeData& b));
    
    ValidationStatus validationStatus;
    float scrollPercentage;
    float lastScale;
    float shortestFrameDuration;
    float longestFrameDuration;
    float splitterHeight;
    bool lastClickedItemWasFrame;
    std::size_t lastClickedItemID;
    std::size_t threadWithSelectedItem;
    std::string errorMessage;
    std::vector<std::int32_t> maxDepths;
    
    std::unordered_map<ScopeKey, ScopeStats> scopeStats;
    std::vector<InsertOnlyIntervalTree<FullEventData>> intervalTrees;
    std::vector<FullEventData> drawnIntervals;
    std::vector<FullScopeData> sortedScopes;
    
    ImVec2 hoveredItemCoordinates;
    static constexpr std::size_t TextBufferSize = 512;
    std::array<char, TextBufferSize> primaryTextBuffer;
    std::array<char, TextBufferSize> secondaryTextBuffer;
    std::array<char, TextBufferSize> filterTextBuffer;
};

#endif // IYFT_PROFILER_WITH_IMGUI

/// \brief Contains results that were recorded by the ThreadProfiler.
class ProfilerResults {
public:
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
    /// \return A ProfilerResults instance or a nullptr if file reading failed.
    static std::unique_ptr<ProfilerResults> LoadFromFile(const std::string& path);
    
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
    ///
    /// \param threadID The ID of the thread. Must be less than getThreadCount()
    const std::deque<RecordedEvent>& getEvents(std::size_t threadID) const {
        return events[threadID];
    }
    
    /// \brief Access the ScopeInfo container.
    const std::unordered_map<ScopeKey, ScopeInfo>& getScopes() const {
        return scopes;
    }
    
    /// \brief Access the tag data container.
    const std::unordered_map<std::uint32_t, TagNameAndColor>& getTags() const {
        return tags;
    }
    
    /// \brief Checks if frame data is missing.
    ///
    /// If this is true (may happen if the profiler didn't run for the whole frame or
    /// if you didn't call IYFT_PROFILER_NEXT_FRAME), the frame deque will contain a 
    /// single artificial frame that starts together with the earliest recorded event
    /// and ends together with the last recorded event. If hasAnyRecords() is false
    /// as well, the start and the end will be equal to 0 and 1.
    bool isFrameDataMissing() const {
        return frameDataMissing;
    }
    
    /// \brief Checks if this ProfilerResults contains any records.
    ///
    /// If this is false, no events were recorded and the event deques will be empty.
    /// This may happen if you forgot to call IYFT_PROFILER_SET_RECORDING(true), 
    /// disabled the profiler before it could record any data or if you didn't 
    /// instrument your code with IYFT_PROFILE calls. Depending on the cause, the scope
    /// data may still be available.
    bool hasAnyRecords() const {
        return anyRecords;
    }
    
    /// \brief Draws the results in Imgui
    ProfilerDrawResult drawInImGui(float scale);
    
    /// \brief Returns the number of threads that were profiled
    std::size_t getThreadCount() const {
        return threadNames.size();
    }
    
    /// \brief Returns the name of a profiled thread
    ///
    /// \param threadID The ID of the thread. Must be less than getThreadCount()
    const std::string& getThreadName(std::size_t threadID) const {
        return threadNames[threadID];
    }
    
    /// \brief Minimal value that can be passed to drawInImGui()
    static float GetMinScale() {
        return 0.2f;
    }
    
    /// \brief Maximum value that can be passed to drawInImGui()
    static float GetMaxScale() {
        return 15.0f;
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
    
#ifdef IYFT_PROFILER_WITH_IMGUI
    std::unique_ptr<ProfilerResultDrawData> drawData;
#endif // IYFT_PROFILER_WITH_IMGUI
}; 

static_assert(std::is_move_assignable<ProfilerResults>::value && std::is_move_constructible<ProfilerResults>::value,
              "ProfilerResults must be moveable");
}

#endif // IYFT_THREAD_PROFILER_CORE_HPP

#if defined IYFT_THREAD_PROFILER_IMPLEMENTATION && !defined IYFT_THREAD_PROFILER_IMPLEMENTATION_INCLUDED
#define IYFT_THREAD_PROFILER_IMPLEMENTATION_INCLUDED

#include <chrono>
#include <mutex>
#include <stdexcept>
#include <cstring>
#include <thread>

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sstream>

namespace iyft {

class ThreadIDAssigner {
public:
    ThreadIDAssigner() : counter(0) {
        for (std::size_t i = 0; i < IYFT_THREAD_PROFILER_MAX_THREAD_COUNT; ++i) {
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
    std::array<std::string, IYFT_THREAD_PROFILER_MAX_THREAD_COUNT> names;
};

static ThreadIDAssigner ThreadIDAssigner;

const std::size_t emptyID = static_cast<std::size_t>(-1);
static thread_local std::size_t CurrentThreadID = emptyID;
static thread_local std::string CurrentThreadName = "";

void ThreadIDAssigner::assignNext(const char* name) {
    std::lock_guard<std::mutex> lock(mutex);
    
    CurrentThreadID = counter;
    
    if (CurrentThreadID >= IYFT_THREAD_PROFILER_MAX_THREAD_COUNT) {
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

void MarkNextFrame() {
    GetThreadProfiler().nextFrame();
}

ScopeInfo& InsertScopeInfo(const char* scopeName, const char* identifier, const char* functionName, const char* fileName, std::uint32_t line, ProfilerTag tag) {
    return GetThreadProfiler().insertScopeInfo(scopeName, identifier, functionName, fileName, line, tag);
}

void InsertScopeStart(const ScopeInfo& info) {
    GetThreadProfiler().insertScopeStart(info);
}

void InsertScopeEnd(const ScopeInfo& info) {
    GetThreadProfiler().insertScopeEnd(info);
}

void SetRecording(bool recording) {
    GetThreadProfiler().setRecording(recording);
}

ProfilerStatus GetStatus() {
    return iyft::GetThreadProfiler().isRecording() ? iyft::ProfilerStatus::EnabledAndRecording : iyft::ProfilerStatus::EnabledAndNotRecording;
}

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
        TagNameAndColor nameAndColor(GetTagName(tag), GetTagColor(tag));
        
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
        
        IYFT_ASSERT(first != std::chrono::nanoseconds::max());
        IYFT_ASSERT(last != std::chrono::nanoseconds::min());
        
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
    
#ifdef IYFT_PROFILER_WITH_COOKIE
    results.withCookie = true;
#else // IYFT_PROFILER_WITH_COOKIE
    results.withCookie = false;
#endif // IYFT_PROFILER_WITH_COOKIE
    
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

std::unique_ptr<ProfilerResults> ProfilerResults::LoadFromFile(const std::string& path) {
    // Warn about a file with cookies when no cookie support is enabled
    std::unique_ptr<ProfilerResults> pr(new ProfilerResults());
    
    std::ifstream is(path, std::ios::binary);
    
    if (!is.is_open()) {
        return nullptr;
    }
    
    // Magic number
    if (is.get() != 'I' || is.get() != 'Y' || is.get() != 'F' || is.get() != 'R') {
        return nullptr;
    }
    
    // Version and some parameters
    if (is.get() != 1) {
        return nullptr;
    }
    
    pr->frameDataMissing = is.get();
    pr->anyRecords = is.get();
    pr->withCookie = is.get();
    
    // Thread names
    const std::uint64_t threadCount = ReadUInt64(is);
    pr->threadNames.reserve(threadCount);
    
    for (std::uint64_t i = 0; i < threadCount; ++i) {
        pr->threadNames.push_back(ReadString(is));
    }

    // Frames
    const std::uint64_t frameCount = ReadUInt64(is);
    
    for (std::uint64_t i = 0; i < frameCount; ++i) {
        std::uint64_t frameNumber = ReadUInt64(is);
        std::chrono::nanoseconds start = ReadNanos(is);
        std::chrono::nanoseconds end = ReadNanos(is);
        
        FrameData frame(frameNumber, start);
        frame.setEnd(end);
        
        pr->frames.emplace_back(std::move(frame));
    }
    
    // Tags
    const std::uint64_t tagCount = ReadUInt64(is);
    pr->tags.reserve(tagCount);
    
    for (std::uint64_t i = 0; i < tagCount; ++i) {
        std::uint32_t tagID = ReadUInt32(is);
        IYFT_ASSERT(i == tagID);
        
        std::string name = ReadString(is);
        
        
        const std::uint8_t r = ReadUInt8(is);
        const std::uint8_t g = ReadUInt8(is);
        const std::uint8_t b = ReadUInt8(is);
        const std::uint8_t a = ReadUInt8(is);
        ScopeColor color(r, g, b, a);
        
        TagNameAndColor nameAndColor(std::move(name), std::move(color));
        
        pr->tags.emplace(tagID, std::move(nameAndColor));
    }
    
    // Scope info
    const std::uint64_t scopeCount = ReadUInt64(is);
    pr->scopes.reserve(scopeCount);
    
    for (std::uint64_t i = 0; i < scopeCount; ++i) {
        const ScopeKey key(ReadUInt32(is));
        
        const ProfilerTag tag = static_cast<ProfilerTag>(ReadUInt32(is));
        const std::string name = ReadString(is);
        const std::string functionName = ReadString(is);
        const std::string fileName = ReadString(is);
        const std::uint32_t lineNumber = ReadUInt32(is);
        
        ScopeInfo scopeInfo(key, name, functionName, fileName, lineNumber, tag);
        pr->scopes.emplace(key, std::move(scopeInfo));
    }
    
    // Events for each thread
    pr->events.resize(threadCount);
    for (std::uint64_t i = 0; i < threadCount; ++i) {
        const std::uint64_t eventCount = ReadUInt64(is);
        
        std::deque<RecordedEvent>& threadData = pr->events[i];
        for (std::uint64_t j = 0; j < eventCount; ++j) {
            const ScopeKey key(ReadUInt32(is));
            
            const std::int32_t depth = ReadInt32(is);
            const std::chrono::nanoseconds start = ReadNanos(is);
            const std::chrono::nanoseconds end = ReadNanos(is);
            
            RecordedEvent event(key, depth, start);
            event.setEnd(end);
#ifdef IYFT_PROFILER_WITH_COOKIE
            const std::uint64_t cookie = 0;
            if (pr->withCookie) {
                cookie = ReadUInt64(is);
            }
            event.setCookie(cookie);
#else // IYFT_PROFILER_WITH_COOKIE
            if (pr->withCookie) {
                ReadUInt64(is);
            }
#endif // IYFT_PROFILER_WITH_COOKIE

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
    
    IYFT_ASSERT(threadNames.size() == events.size());
    
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
#ifdef IYFT_PROFILER_WITH_COOKIE
            WriteUInt64(e.getCookie());
#endif // IYFT_PROFILER_WITH_COOKIE
        }
    }
    
    return true;
}

static void writeFrameData(std::stringstream& ss, const FrameData& frame) {
    const IYFT_THREAD_TEXT_OUTPUT_DURATION duration = frame.getDuration();
    ss << "  FRAME: " << frame.getNumber() 
       << "; Duration: " << duration.count() << IYFT_THREAD_TEXT_OUTPUT_NAME "\n";
}

std::string ProfilerResults::writeToString() const {
    std::stringstream ss;
    
    IYFT_ASSERT(frames.size() > 0);
    
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
            IYFT_ASSERT(result != scopes.end());
            
            const ScopeInfo& info = result->second;
            
            const IYFT_THREAD_TEXT_OUTPUT_DURATION d = e.getDuration();
            
            const std::size_t offset = static_cast<std::size_t>(e.getDepth() * 2 + 4);
            const std::string depthOffset(offset, ' ');
            ss << depthOffset << "SCOPE: " << info.getName()
#ifdef IYFT_PROFILER_WITH_COOKIE
               << "; Cookie: " << e.getCookie()
#endif // IYFT_PROFILER_WITH_COOKIE
               << "; Function: " << info.getFunctionName()
               << "; Duration: " << d.count() << IYFT_THREAD_TEXT_OUTPUT_NAME "\n";
        }
    }
    
    return ss.str();
}

#ifdef IYFT_PROFILER_WITH_IMGUI
enum class ProfilerItemStatus : unsigned int {
    NoInteraction = 0x0,
    Hovered = 0x01,
    HoveredAndNeedsHighlight = 0x02,
    Clicked = 0x04
};

inline ProfilerItemStatus operator|= (ProfilerItemStatus &a, ProfilerItemStatus b) {
    return (ProfilerItemStatus&)((unsigned int&)(a) |= static_cast<unsigned int>(b));
}

inline ProfilerItemStatus operator&(ProfilerItemStatus a, ProfilerItemStatus b) {
    return static_cast<ProfilerItemStatus>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

constexpr float MIN_PIXELS_PER_MS = 150.0f;
constexpr float PADDING = 5.0f;

/// Based on https://stackoverflow.com/a/1855903/1723459
inline static ImColor DetermineFontColorFromBackgroundColor(const ImColor& backgroundColor) {
    const ImVec4 background = backgroundColor;
    const float perceptiveLuminance = 0.299f * background.x + 0.587f * background.y + 0.114f * background.z;
    
    return (perceptiveLuminance > 0.5f) ? ImColor(0.0f, 0.0f, 0.0f) : ImColor(1.0f, 1.0f, 1.0f); 
}

inline static ProfilerItemStatus DrawRectWithText(ImDrawList* drawList, const ImVec2& cursor, float rectHeight, float verticalOffset,
                                    float pixelsPerMs, float start, float end, const ImColor& rectColor, std::size_t currentID,
                                    std::size_t& clickedItemID, ImVec2& hoveredItemCoordinates, char* currentText, char* delayedText,
                                    int bufferSize, const char* format, ...) {
    const float startX = cursor.x + start * pixelsPerMs;
    const float endX = cursor.x + end * pixelsPerMs;
    
    const ImVec2 upperLeft(startX, cursor.y + rectHeight + verticalOffset);
    const ImVec2 lowerRight(endX, cursor.y + verticalOffset);
    const ImVec2 textPos(upperLeft.x + PADDING, lowerRight.y);
    const ImVec4 textClip(textPos.x, textPos.y, lowerRight.x, upperLeft.y);
    const ImColor textAndBorderColor(DetermineFontColorFromBackgroundColor(rectColor));
    
    ImGui::SetCursorScreenPos(ImVec2(upperLeft.x, lowerRight.y));
    
    const float width = lowerRight.x - upperLeft.x;
    const float height = upperLeft.y - lowerRight.y;
    ImGui::Dummy(ImVec2(width, height));
    
    va_list args;
    va_start(args, format);
    int symbolCount = std::vsnprintf(currentText, bufferSize, format, args);
    va_end(args);
    
    ProfilerItemStatus status = ProfilerItemStatus::NoInteraction;
    // If an item is clicked, it must also be hovered
    if (ImGui::IsItemHovered()) {
        // TODO should I get the color and make it brighter/darker instead?
        drawList->AddRectFilled(upperLeft, lowerRight, ImColor(1.0f, 0.0f, 0.0f));
        
        const ImVec2 textSize = ImGui::CalcTextSize(currentText);
        
        if ((startX + textSize.x) >= endX) {
            std::strncpy(delayedText, currentText, symbolCount + 1);
            hoveredItemCoordinates = textPos;
            
            status = ProfilerItemStatus::HoveredAndNeedsHighlight;
        } else {
            drawList->AddText(textPos, ImColor(1.0f, 1.0f, 1.0f), currentText, currentText + symbolCount);
            status = ProfilerItemStatus::Hovered;
        }
        
        if (ImGui::IsItemClicked()) {
            clickedItemID = currentID;
            status |= ProfilerItemStatus::Clicked;
        }
    } else {
        drawList->AddRectFilled(upperLeft, lowerRight, rectColor);
        drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, textAndBorderColor, currentText, currentText + symbolCount, 0.0f, &textClip);
        
        status = ProfilerItemStatus::NoInteraction;
    }
    drawList->AddRect(upperLeft, lowerRight, textAndBorderColor);
    
    return status;
}

inline static ImColor ImColorFromScopeColor(const ScopeColor& sc) {
    return ImColor(sc.getRed(), sc.getGreen(), sc.getBlue(), sc.getAlpha());
}

inline static float FrameDurationGetter(void* data, int idx) {
    const std::deque<FrameData>* frames = static_cast<std::deque<FrameData>*>(data);
    std::chrono::duration<float, std::milli> duration = (*frames)[idx].getDuration();
    return duration.count();
}

inline static void DrawHoveredText(ImDrawList* drawList, const char* textBuffer, const ImVec2& coordinates, float height) {
    const ImVec2 textSize = ImGui::CalcTextSize(textBuffer);
    const ImVec2 rectStart(coordinates.x - PADDING, coordinates.y + height);
    const ImVec2 rectEnd(rectStart.x + textSize.x + 2 * PADDING, coordinates.y);
    
    drawList->AddRectFilled(rectStart, rectEnd, ImColor(0.0f, 0.0f, 0.0f, 0.92f));
    drawList->AddText(coordinates, ImColor(1.0f, 1.0f, 1.0f), textBuffer);
}

inline static void DisplayTimingData(const TimedProfilerObject& timedObject, std::chrono::nanoseconds startOffset) {
    std::chrono::nanoseconds start = timedObject.getStart() - startOffset;
    std::chrono::nanoseconds end = timedObject.getEnd() - startOffset;
    std::chrono::nanoseconds duration = timedObject.getDuration();
    
    const float startMs = std::chrono::duration<float, std::milli>(start).count();
    const float endMs = std::chrono::duration<float, std::milli>(end).count();
    const float durationMs = std::chrono::duration<float, std::milli>(duration).count();
    
    ImGui::Text("Start: %fms\nEnd: %fms\nDuration: %fms", startMs, endMs, durationMs);
}

inline static float CalculateThreadRowHeight(float eventHeight, float eventSpacing, const std::int32_t rowCount) {
    return (eventHeight + eventSpacing) * (rowCount + 1) + eventSpacing;
}

inline static std::uint64_t ComputeFrameNumber(const std::deque<FrameData>& frames, std::chrono::nanoseconds value, std::uint64_t firstFrameNumber) {
    auto frameResult = std::lower_bound(frames.begin(), frames.end(), value, [](const FrameData& lhs, const std::chrono::nanoseconds& rhs){
        return lhs.getEnd() < rhs;
    });
    
    if (frameResult == frames.end()) {
        return frames.begin()->getNumber() - firstFrameNumber;
    } else {
        return frameResult->getNumber() - firstFrameNumber;
    }
}

template <typename T>
std::string InsertOnlyIntervalTree<T>::print() const {
    std::stringstream ss;
    printTreePreOrder(ss, root, 0);
    return ss.str();
}

template <typename T>
void InsertOnlyIntervalTree<T>::printTreePreOrder(std::stringstream& ss, const Node* node, std::size_t depth) const {
    if (node == nullptr) {
        return;
    }
    
    const std::string offsetString(depth, ' ');
    
    ss << offsetString;
    node->item.print(ss);
    ss << " MAX: " << node->maximum.count();
    
    if (node->sibling != nullptr) {
        ss << "; SIBLINGS: ";
        
        const Node* listItem = node->sibling;
        while (listItem != nullptr) {
            listItem->item.print(ss);
            
            listItem = listItem->sibling;
            if (listItem != nullptr) {
                ss << ", ";
            }
        }
    }
    
    ss << "\n";
    
    const std::size_t currentDepth = ++depth;
    
    printTreePreOrder(ss, node->left, currentDepth);
    printTreePreOrder(ss, node->right, currentDepth);
}

void ProfilerResultDrawData::drawScopeInfoColumnName(const char* name, std::vector<FullScopeData>& sortedScopes,
                                                     bool (*const ascendingSort)(const FullScopeData& a, const FullScopeData& b),
                                                     bool (*const descendingSort)(const FullScopeData& a, const FullScopeData& b)) {
    ImGui::PushID(ImGui::GetColumnIndex());
    if (ImGui::ArrowButton("Up", ImGuiDir_Up)) {
        std::sort(sortedScopes.begin(), sortedScopes.end(), ascendingSort);
    }
    
    ImGui::SameLine();
    
    if (ImGui::ArrowButton("Down", ImGuiDir_Down)) {
        std::sort(sortedScopes.begin(), sortedScopes.end(), descendingSort);
    }
    ImGui::PopID();
    
    ImGui::SameLine();
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", name);
}

ProfilerResultDrawData::ProfilerResultDrawData(const ProfilerResults* results) 
    : results(results),
      validationStatus(ValidationStatus::Pending),
      scrollPercentage(0.0f),
      lastScale(1.0f),
      shortestFrameDuration(FLT_MAX),
      longestFrameDuration(0.0f),
      splitterHeight(250.0f),
      lastClickedItemWasFrame(true),
      lastClickedItemID(0),
      threadWithSelectedItem(0),
      primaryTextBuffer(),
      secondaryTextBuffer(),
      filterTextBuffer()
{
    if (!results->hasAnyRecords()) {
        errorMessage = "No necords. Did you instrument the code and start the recording?";
        validationStatus = ValidationStatus::Invalid;
        
        return;
    }
    
    const std::deque<FrameData>& frames = results->getFrames();
    if (frames.size() > 1) {
        auto iter = frames.begin();
        std::uint64_t lastFrameNumber = iter->getNumber();
        iter++;
        
        for (; iter != frames.end(); iter++) {
            const std::uint64_t currentFrameNumber = iter->getNumber();
            if (currentFrameNumber != (lastFrameNumber + 1)) {
                errorMessage = "The recorded frames are not sequential.";
                validationStatus = ValidationStatus::Invalid;
                
                return;
            }
            
            const float currentFrameDuration = std::chrono::duration<float, std::milli>(iter->getDuration()).count();
            shortestFrameDuration = std::min(shortestFrameDuration, currentFrameDuration);
            longestFrameDuration = std::max(longestFrameDuration, currentFrameDuration);
            
            lastFrameNumber++;
        }
    } else if (frames.size() == 1) {
        const float currentFrameDuration = std::chrono::duration<float, std::milli>(frames[0].getDuration()).count();
        shortestFrameDuration = currentFrameDuration;
        longestFrameDuration = currentFrameDuration;
    } else {
        errorMessage = "No frames were recorded.";
        validationStatus = ValidationStatus::Invalid;
        
        
        return;
    }
    
    const std::size_t threadCount = results->getThreadCount();
    
    maxDepths.resize(threadCount, 0);
    for (std::size_t e = 0; e < threadCount; ++e) {
        const std::deque<RecordedEvent>& records = results->getEvents(e);
        
        for (const RecordedEvent& r : records) {
            maxDepths[e] = std::max(maxDepths[e], r.getDepth());
        }
    }
    
    const std::unordered_map<ScopeKey, ScopeInfo>& scopes = results->getScopes();
    const std::unordered_map<std::uint32_t, TagNameAndColor>& tags = results->getTags();
    
    intervalTrees.reserve(threadCount);
    for (std::size_t i = 0; i < threadCount; ++i) {
        const std::deque<RecordedEvent>& records = results->getEvents(i);
        
        intervalTrees.emplace_back(records.size());
        InsertOnlyIntervalTree<FullEventData>& currentTree = intervalTrees.back();
        
        auto eventIter = records.begin();
        for (std::size_t j = 0; j < records.size(); ++j) {
            const RecordedEvent& e = *eventIter;
            eventIter++;
            
//             if (!e.isValid() || !e.isComplete()) {
//                 continue;
//             }
            
            auto scopeResult = scopes.find(e.getKey());
            if (scopeResult == scopes.end()) {
                errorMessage = "Missing scope information.";
                validationStatus = ValidationStatus::Invalid;
                
                return;
            }
            
            const ScopeInfo& scope = scopeResult->second;
            
            auto tagResult = tags.find(static_cast<std::uint32_t>(scope.getTag()));
            if (tagResult == tags.end()) {
                errorMessage = "Missing tag information.";
                validationStatus = ValidationStatus::Invalid;
                
                return;
            }
            
            const TagNameAndColor& tagInfo = tagResult->second;
            
            currentTree.insert(FullEventData(j, &e, &scope, &tagInfo));
            
// std::uint64_t totalCalls;
// std::chrono::duration<float, std::milli> averageCallDuration;
// 
// std::chrono::duration<float, std::milli> minCallDuration;
// std::uint64_t shortestFrameNumber;
// 
// std::chrono::duration<float, std::milli> maxCallDuration;
// std::uint64_t longestFrameNumber;
            // Build the stats
            
            std::uint64_t firstFrameNumber = frames.begin()->getNumber();
            auto statResult = scopeStats.find(e.getKey());
            if (statResult != scopeStats.end()) {
                ScopeStats& stats = statResult->second;
                
                stats.totalCalls += 1;
                stats.averageCallDuration += e.getDuration();
                
                if (e.getDuration() < stats.minCallDuration) {
                    stats.minCallDuration = e.getDuration();
                    stats.shortestFrameNumber = ComputeFrameNumber(frames, e.getStart(), firstFrameNumber);
                }
                
                if (e.getDuration() > stats.maxCallDuration) {
                    stats.maxCallDuration = e.getDuration();
                    stats.longestFrameNumber = ComputeFrameNumber(frames, e.getStart(), firstFrameNumber);
                }
            } else {
                ScopeStats stats;
                
                stats.totalCalls = 1;
                stats.averageCallDuration = e.getDuration();
                
                const std::uint64_t frameNumber = ComputeFrameNumber(frames, e.getStart(), firstFrameNumber);
                
                stats.minCallDuration = e.getDuration();
                stats.shortestFrameNumber = frameNumber;
                
                stats.maxCallDuration = e.getDuration();
                stats.longestFrameNumber = frameNumber;
                
                scopeStats[e.getKey()] = std::move(stats);
            }
        }
        
        currentTree.updateMaximumValues();
    }
    
    for (auto& s : scopeStats) {
        s.second.averageCallDuration /= static_cast<float>(s.second.totalCalls);
    }
    
    sortedScopes.reserve(scopes.size());
    for (const auto& s : scopes) {
        auto tagResult = tags.find(static_cast<std::uint32_t>(s.second.getTag()));
        if (tagResult == tags.end()) {
            errorMessage = "Missing tag information.";
            validationStatus = ValidationStatus::Invalid;
            
            return;
        }
        
        // This will create a default initialized ScopeStats instance if one is missing
        const ScopeStats& stats = scopeStats[s.first];
        
        sortedScopes.emplace_back(&(s.second), &(tagResult->second), &stats);
    }
    
    std::sort(sortedScopes.begin(), sortedScopes.end(), [](const FullScopeData& a, const FullScopeData& b){
        return a.scopeInfo->getName() < b.scopeInfo->getName();
    });
    
    errorMessage = "";
    validationStatus = ValidationStatus::Validated;
}
    
ProfilerDrawResult ProfilerResultDrawData::draw(float scale) {
    // Splitter behaviour based on https://github.com/ocornut/imgui/issues/125#issuecomment-135775009
    // I added some extra style var pushing and popping because spacing around event items disappears
    // otherwise
    
    ImGui::BeginChild("ProfilerWrapper");
    if (validationStatus != ValidationStatus::Validated) {
        ImGui::Text("Profiler results are invalid. ERROR:%s", errorMessage.c_str());
        
        ImGui::EndChild();
        return ProfilerDrawResult::DrawingFailed;
    }
    
    const ImVec2 defaultItemSpacing = ImGui::GetStyle().ItemSpacing;
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
    ImGui::BeginChild("ProfilerMainContent", ImVec2(0, splitterHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, defaultItemSpacing);
    
    scale = std::max(ProfilerResults::GetMinScale(), std::min(scale, ProfilerResults::GetMaxScale()));
    
    const std::deque<FrameData>& frames = results->getFrames();
    
    const ImVec2 position = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const ImVec4 clipRect(position.x, position.y, position.x + size.x, position.y + size.y);
    
    // Durations and frame counts used in further calculations
    const std::uint64_t frameCount = frames.size();
    const std::uint64_t firstFrameNumber = frames.front().getNumber();
    const std::chrono::nanoseconds start = frames.front().getStart();
    const std::chrono::nanoseconds end = frames.back().getEnd();
    const std::chrono::nanoseconds durationNanos = end - start;
    const float recordDuration = std::chrono::duration<float, std::milli>(durationNanos).count();
    //const std::size_t recordDurationCeil = std::ceil(recordDuration);
    const float msPerFrame = recordDuration / frameCount;
    const float FPS = 1000.0f / msPerFrame;
    
    // Determine the width of the drawing area.
    const float pixelsPerMs = MIN_PIXELS_PER_MS * scale;
    const float totalWidth = recordDuration * pixelsPerMs;//std::max(frameDisplayWidth, parentSize.x);
    
    const float lineWithSpacing = ImGui::GetTextLineHeightWithSpacing();
    const float lineWithoutSpacing = ImGui::GetTextLineHeight();
    const float lineSpacing = lineWithSpacing - lineWithoutSpacing;
    
    //const float scrollMax = ImGui::GetScrollMaxX();
    const float scrollCurrent = ImGui::GetScrollX();
    
    // TODO FIXME this causes flickering and I can't seem to solve it
    if (scale != lastScale) {
        lastScale = scale;
        ImGui::SetScrollX(totalWidth * scrollPercentage);
    } else {
        scrollPercentage = scrollCurrent / totalWidth;
    }
    
    ImGui::BeginChild("ScrollingRegion", ImVec2(totalWidth, 0), false);
    
    const float leftmostPoint = ImGui::GetCursorScreenPos().x;
    const float threadNameTextX = clipRect.x + ImGui::GetStyle().ItemSpacing.x;
    
    // Compute the cutoff points for horizontal clipping, by determining the first and the last visible nanosecond
    // Since all frames and events are sorted, we can use these values together with a binary search (lower_bound)
    // to clip the ranges extremely quickly.
    const std::int64_t startNanosOffset = static_cast<std::int64_t>(durationNanos.count() * ((clipRect.x - leftmostPoint) / totalWidth));
    const std::int64_t endNanosOffset = static_cast<std::int64_t>(durationNanos.count() * ((clipRect.z - leftmostPoint) / totalWidth));
    const std::chrono::nanoseconds firstVisibleNanosecond(start + std::chrono::nanoseconds(startNanosOffset));
    const std::chrono::nanoseconds lastVisibleNanosecond(start + std::chrono::nanoseconds(endNanosOffset));
    
    // Make sure the text is always visible
    ImGui::SetCursorScreenPos(ImVec2(threadNameTextX, ImGui::GetCursorScreenPos().y));
//     ImGui::Text("%f;%f", leftmostPoint, startNanosOffset);
    ImGui::Text("Frames: %lu. Total time: %.2f ms; %.2f ms/frame (%.2f FPS)", frameCount, recordDuration, msPerFrame, FPS);
    if (results->isFrameDataMissing()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "(NO FRAME DATA)");
    }
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 tickDrawStart = ImGui::GetCursorScreenPos();
    
    const float tallTickHeight = lineWithSpacing * 1.5f;
    const float shortTickVerticalOffset = lineWithSpacing;
    const ImColor tallTickColor(1.0f, 0.0f, 0.0f, 1.0f);
    const ImColor shortTickColor(1.0f, 1.0f, 0.0f, 1.0f);
    const ImColor frameNumberColor(1.0f, 0.0f, 0.0f, 1.0f);
    
    const std::chrono::milliseconds firstTick = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(startNanosOffset));
    const std::chrono::milliseconds lastTick = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(endNanosOffset));
    for (std::chrono::milliseconds i = firstTick; i <= lastTick; ++i) {
        const float tallTickXCoordinate = tickDrawStart.x + (pixelsPerMs * i.count());
        
        const ImVec2 tallTickStart(tallTickXCoordinate, tickDrawStart.y);
        const ImVec2 tallTickEnd(tallTickXCoordinate, tickDrawStart.y + tallTickHeight);
        
        drawList->AddLine(tallTickStart, tallTickEnd, tallTickColor);
        
        int symbolCount = std::snprintf(primaryTextBuffer.data(), primaryTextBuffer.size(), "%zu", i.count());
        const ImVec2 textStart(tallTickStart.x + PADDING, tallTickStart.y);
        drawList->AddText(textStart, frameNumberColor, primaryTextBuffer.data(), primaryTextBuffer.data() + symbolCount);
        
        for (std::size_t j = 1; j < 10; ++j) {
            const float shortTickXCoordinate = tallTickXCoordinate + (pixelsPerMs * 0.1f * j);
            const ImVec2 shortTickStart = ImVec2(shortTickXCoordinate, tickDrawStart.y + shortTickVerticalOffset);
            const ImVec2 shortTickEnd = ImVec2(shortTickXCoordinate, tickDrawStart.y + tallTickHeight);
            
            drawList->AddLine(shortTickStart, shortTickEnd, shortTickColor);
        }
    }
    
    ImGui::SetCursorScreenPos(ImVec2(tickDrawStart.x, tickDrawStart.y + tallTickHeight));
    ImGui::Spacing();
    
    const ImVec2 frameDrawStart = ImGui::GetCursorScreenPos();
    
    const ImColor frameColorOdd(0.0f, 0.0f, 1.0f, 1.0f);
    const ImColor frameColorEven(0.0f, 1.0f, 1.0f, 1.0f);
    const float frameHeight = lineWithoutSpacing;
    
    // Find the first frame that ends after the start of the test frame. 
    const auto firstFrame = std::lower_bound(frames.begin(), frames.end(), firstVisibleNanosecond, [](const FrameData& lhs, const std::chrono::nanoseconds& rhs){
        return lhs.getEnd() < rhs;
    });
    
    // Find the first frame that starts after the end of the test frame.
    const auto lastFrame = std::lower_bound(frames.begin(), frames.end(), lastVisibleNanosecond, [](const FrameData& lhs, const std::chrono::nanoseconds& rhs){
        return lhs.getStart() < rhs;
    });
    
    ProfilerItemStatus frameStatus = ProfilerItemStatus::NoInteraction;
    for (auto iter = firstFrame; iter != lastFrame;) {
        const FrameData& fd = *iter;
        const float frameStartMs = std::chrono::duration<float, std::milli>(fd.getStart() - start).count();
        const float frameEndMs = std::chrono::duration<float, std::milli>(fd.getEnd() - start).count();
        
        // Makes more sense to count from zero
        const std::uint64_t currentFrameNumber = fd.getNumber() - firstFrameNumber;
        const float frameDuration = std::chrono::duration<float, std::milli>(fd.getDuration()).count();
        const std::size_t itemID = std::distance(frames.begin(), iter);
        frameStatus |= DrawRectWithText(drawList, frameDrawStart, frameHeight, 0.0f, pixelsPerMs,
                                        frameStartMs, frameEndMs, (fd.getNumber() % 2 ? frameColorOdd : frameColorEven), itemID, lastClickedItemID,
                                        hoveredItemCoordinates, primaryTextBuffer.data(), secondaryTextBuffer.data(), TextBufferSize,
                                        "%" PRIu64 " (%f ms)", currentFrameNumber, frameDuration);
        
        ++iter;
    }
    
    if (static_cast<bool>(frameStatus & ProfilerItemStatus::HoveredAndNeedsHighlight)) {
        DrawHoveredText(drawList, secondaryTextBuffer.data(), hoveredItemCoordinates, lineWithoutSpacing);
    }
    
    if (static_cast<bool>(frameStatus & ProfilerItemStatus::Clicked)) {
        lastClickedItemWasFrame = true;
    }
    
    ImGui::SetCursorScreenPos(ImVec2(frameDrawStart.x, frameDrawStart.y + frameHeight));
    ImGui::Spacing();
    
    ImGui::Separator();
    
//     const int clipRectStartX = frameDrawStart.x;
//     const int clipRectEndX = frameDrawStart.x + totalWidth;
//     const int clipRectStartY = frameDrawStart.y + frameHeight;
//     const int clipRectEndY = frameDrawStart.y + frameHeight + 100.0f;
//     drawList->PushClipRect(ImVec2(clipRectStartX, clipRectStartY), ImVec2(clipRectEndX, clipRectEndY), false);
    
    const float recordedEventHeight = lineWithoutSpacing;
    const float recordedEventSpacing = lineSpacing;
    
    ProfilerItemStatus allThreadEventStatus = ProfilerItemStatus::NoInteraction;
    for (std::size_t i = 0; i < results->getThreadCount(); ++i) {
        // Make sure the name is always visible
        ImGui::SetCursorScreenPos(ImVec2(threadNameTextX, ImGui::GetCursorScreenPos().y));
        const std::string& threadName = results->getThreadName(i);
        ImGui::Text("Thread: %s", threadName.c_str());
        
        drawnIntervals.clear();
        
        TimedProfilerObject checkInterval(firstVisibleNanosecond);
        checkInterval.setEnd(lastVisibleNanosecond);
        intervalTrees[i].findIntervals(FullEventData(0, &checkInterval, nullptr, nullptr), drawnIntervals);
        
        const ImVec2 threadRecordCursorPos = ImGui::GetCursorScreenPos();
        const ImVec2 threadRecordDrawStart(threadRecordCursorPos.x, threadRecordCursorPos.y + recordedEventSpacing);
        const float totalHeight = CalculateThreadRowHeight(recordedEventHeight, recordedEventSpacing, maxDepths[i]);
        
        drawList->AddRectFilled(threadRecordCursorPos, ImVec2(totalWidth, threadRecordCursorPos.y + totalHeight), ImColor(0.0f, 0.0f, 0.0f, 0.5f));
        
        ProfilerItemStatus eventStatus = ProfilerItemStatus::NoInteraction;
        
        for (const FullEventData& e : drawnIntervals) {
            const RecordedEvent* event = dynamic_cast<const RecordedEvent*>(e.event);
            const ScopeInfo* scope = e.scope;
            const TagNameAndColor* nameAndColor = e.nameAndColor;
            const std::size_t itemID = e.id;
            
            const float eventStartMs = std::chrono::duration<float, std::milli>(event->getStart() - start).count();
            const float eventEndMs = std::chrono::duration<float, std::milli>(event->getEnd() - start).count();
            const float eventDuration = std::chrono::duration<float, std::milli>(event->getDuration()).count();
            
            const ImColor itemColor = ImColorFromScopeColor(nameAndColor->getColor());
            eventStatus |= DrawRectWithText(drawList, threadRecordDrawStart, recordedEventHeight, (recordedEventHeight + recordedEventSpacing) * event->getDepth(), pixelsPerMs,
                                            eventStartMs, eventEndMs, itemColor, itemID, lastClickedItemID, hoveredItemCoordinates, primaryTextBuffer.data(), secondaryTextBuffer.data(),
                                            TextBufferSize, "%s (%f ms)", scope->getName().c_str(), eventDuration);
        }
        
        if (static_cast<bool>(eventStatus & ProfilerItemStatus::Clicked)) {
            threadWithSelectedItem = i;
            lastClickedItemWasFrame = false;
        }
        
        allThreadEventStatus |= eventStatus;
        
        ImGui::SetCursorScreenPos(ImVec2(threadRecordDrawStart.x, threadRecordDrawStart.y + totalHeight));
        ImGui::Spacing();
        ImGui::Separator();
    }
    
    if (ImGui::IsWindowHovered()) {
        const ImVec2 mouseCoords = ImGui::GetMousePos();
//         const char* textBuffer = "Hey";
//         const ImVec2 textSize = ImGui::CalcTextSize(textBuffer);
//         const ImVec2 rectStart(mouseCoords.x - textSize.x - 2 * PADDING, mouseCoords.y);
//         const ImVec2 rectEnd(mouseCoords.x, mouseCoords.y + lineWithoutSpacing);
//         
//         drawList->AddRectFilled(rectStart, rectEnd, ImColor(0.0f, 0.0f, 0.0f, 0.92f));
//         drawList->AddText(ImVec2(rectStart.x + PADDING, rectStart.y), ImColor(1.0f, 1.0f, 1.0f), textBuffer);
        
        drawList->AddLine(ImVec2(mouseCoords.x, tickDrawStart.y + tallTickHeight), ImVec2(mouseCoords.x, ImGui::GetCursorScreenPos().y), ImColor(1.0f, 0.0f, 0.0f, 0.5f), 3.0f);
    }
    
    // Draw the names of hovered events separately to make sure they are visible on top of evrything else.
    if (static_cast<bool>(allThreadEventStatus & ProfilerItemStatus::HoveredAndNeedsHighlight)) {
        DrawHoveredText(drawList, secondaryTextBuffer.data(), hoveredItemCoordinates, lineWithoutSpacing);
    }
    
    //drawList->PopClipRect();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    
    ImGui::InvisibleButton("horizontalSplitter", ImVec2(-1,8.0f));
    if (ImGui::IsItemActive()) {
        splitterHeight += ImGui::GetIO().MouseDelta.y;
    }
    
    ImGui::PopStyleVar();
    
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    
    ImGui::BeginChild("ProfilerStatsChild", ImVec2(0, 0), true);
    const ImVec2 plotSize(size.x, 2 * lineWithSpacing);
    
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Frame duration plot")) {
        // I hate this const cast, but it's mandatory here. Besides, we're not modifying anything.
        ImGui::PlotLines("##FrameDurations", FrameDurationGetter, const_cast<std::deque<FrameData>*>(&frames), frames.size(), 0, nullptr, shortestFrameDuration, longestFrameDuration, plotSize);
    }
    
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Selection information")) {
        if (lastClickedItemWasFrame) {
            const FrameData& frame = frames[lastClickedItemID];
            
            ImGui::Text("FRAME: %lu", frame.getNumber() - firstFrameNumber);
            DisplayTimingData(frame, start);
        } else {
            const std::deque<RecordedEvent>& threadEvents = results->getEvents(threadWithSelectedItem);
            const RecordedEvent& event = threadEvents[lastClickedItemID];
            
            const std::unordered_map<ScopeKey, ScopeInfo>& scopes = results->getScopes();
            auto scopeResult = scopes.find(event.getKey());
            IYFT_ASSERT(scopeResult != scopes.end());
            const ScopeInfo& scope = scopeResult->second;
            
            ImGui::Text("EVENT: %s", scope.getName().c_str());
            ImGui::Text("Function: %s\nLocation: %s:%u", scope.getFunctionName().c_str(), scope.getFileName().c_str(), scope.getLineNumber());
            
            DisplayTimingData(event, start);
        }
    }
    
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Recorded scopes")) {
        ImGui::InputText("Filter", filterTextBuffer.data(), filterTextBuffer.size());
        const std::size_t filterStringLength = std::strlen(filterTextBuffer.data());
        
        ImGui::Columns(7, "ScopeInfoColumns");
        
        ImGui::Separator();
        
        drawScopeInfoColumnName("Scope", sortedScopes, 
                                [](const FullScopeData& a, const FullScopeData& b) {
                                    return a.scopeInfo->getName() > b.scopeInfo->getName();
                                },
                                [](const FullScopeData& a, const FullScopeData& b) {
                                    return a.scopeInfo->getName() < b.scopeInfo->getName();
                                });
        ImGui::NextColumn();
        
        drawScopeInfoColumnName("Tag", sortedScopes, 
                                [](const FullScopeData& a, const FullScopeData& b) {
                                    return a.tagInfo->getName() > b.tagInfo->getName();
                                },
                                [](const FullScopeData& a, const FullScopeData& b) {
                                    return a.tagInfo->getName() < b.tagInfo->getName();
                                });
        ImGui::NextColumn();
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Function");
        ImGui::NextColumn();
        
        drawScopeInfoColumnName("Total calls", sortedScopes, 
                                [](const FullScopeData& a, const FullScopeData& b) {
                                    return a.stats->totalCalls < b.stats->totalCalls;
                                },
                                [](const FullScopeData& a, const FullScopeData& b) {
                                    return a.stats->totalCalls > b.stats->totalCalls;
                                });
        ImGui::NextColumn();
        
        drawScopeInfoColumnName("Average duration", sortedScopes, 
                                [](const FullScopeData& a, const FullScopeData& b) {
                                    return a.stats->averageCallDuration < b.stats->averageCallDuration;
                                },
                                [](const FullScopeData& a, const FullScopeData& b) {
                                    return a.stats->averageCallDuration > b.stats->averageCallDuration;
                                });
        ImGui::NextColumn();
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Max duration (frame)");
        ImGui::NextColumn();
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Min duration (frame)");
        ImGui::NextColumn();
        
        ImGui::Separator();
        
        for (const auto& s : sortedScopes) {
            if (filterStringLength != 0 && s.scopeInfo->getName().find(filterTextBuffer.data()) == std::string::npos) {
                continue;
            }
            
            ImGui::Selectable(s.scopeInfo->getName().c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
            ImGui::NextColumn();
            
            ImGui::Text("%s", s.tagInfo->getName().c_str());
            ImGui::NextColumn();
            
            ImGui::Text("%s", s.scopeInfo->getFunctionName().c_str());
            ImGui::NextColumn();
            
            ImGui::Text("%lu", s.stats->totalCalls);
            ImGui::NextColumn();
            
            ImGui::Text("%fms", s.stats->averageCallDuration.count());
            ImGui::NextColumn();
            
            ImGui::Text("%fms (%lu)", s.stats->maxCallDuration.count(), s.stats->longestFrameNumber);
            ImGui::NextColumn();
            
            ImGui::Text("%fms (%lu)", s.stats->minCallDuration.count(), s.stats->shortestFrameNumber);
            ImGui::NextColumn();
        }
        
        ImGui::Columns(1, "ScopeInfoColumns");
        ImGui::Separator();
        ImGui::Spacing();
    }
    
    //ImGui::Text("%s", intervalTrees[0].print().c_str());
    ImGui::EndChild();
    ImGui::EndChild();
    
    return ProfilerDrawResult::DrawnSuccessfully;
}

ProfilerDrawResult ProfilerResults::drawInImGui(float scale) {
    if (drawData == nullptr) {
        drawData = std::unique_ptr<ProfilerResultDrawData>(new ProfilerResultDrawData(this));
    }
    
    return drawData->draw(scale);
}

void ProfilerResultDrawData::FullEventData::print(std::stringstream& ss) const {
    ss << "[" << event->getStart().count() << "; " << event->getEnd().count() << "]";
}

#else // IYFT_PROFILER_WITH_IMGUI
ProfilerDrawResult ProfilerResults::drawInImGui(float) {
    return ProfilerDrawResult::ImGuiNotAvailable;
}
#endif // IYFT_PROFILER_WITH_IMGUI
}

#endif // defined IYFT_THREAD_PROFILER_IMPLEMENTATION && !defined IYFT_THREAD_PROFILER_IMPLEMENTATION_INCLUDED
