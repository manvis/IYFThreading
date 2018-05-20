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

/// \file ThreadProfilerSettings.hpp Contains customizable setting.

#ifndef IYF_THREAD_PROFILER_SETTINGS_HPP
#define IYF_THREAD_PROFILER_SETTINGS_HPP

namespace iyft {

// The maximum number of threads that the ThreadProfiler will need to track in your 
// program. Default is 16. Must be >= 1
//#define IYF_THREAD_PROFILER_MAX_THREAD_COUNT 64

// A custom hashing function. Must return a 32 bit integer and take std::string as the
// parameter.
//#define IYF_THREAD_PROFILER_HASH(a) SOME-FUNCTION-HERE

// Either define both or none. This allows you to customize the duration type that's
// used when building the IYF_PROFILER_RESULT_STRING
//#define IYF_THREAD_TEXT_OUTPUT_DURATION std::chrono::duration<double, std::milli>
//#define IYF_THREAD_TEXT_OUTPUT_NAME "ms"

// I used this to debug some event order issues but you may find other uses for it.
// Uncomment this to tag recorded events with monotonically increasing 64 bit integers.
//#define IYF_PROFILER_WITH_COOKIE

/// \brief A list of tags that identify a group of profiled scopes.
///
/// \warning Do not change the underlying type and make sure the values are sequential.
///
/// \warning Do not remove the NoTag and COUNT enum values and do not change their
/// definitions.
enum class ProfilerTag : std::uint32_t {
    NoTag = 0,  ///< Indicates an untagged scope.
    
    // ---- CUSTOM TAG START ----
    // e.g., RenderSystem, SoundSystem, Network, etc.
    // ---- CUSTOM TAG END
    
    COUNT       ///< The total number of tags.
};

/// \brief Return a human readable name for the provided tag.
///
/// \remark Customize the definition of this function to fit your ProfilerTag values.
const char* GetTagName(ProfilerTag tag);

/// \brief Return a color assigned to the provided tag.
///
/// \remark Customize the definition of this function to fit your ProfilerTag values.
ScopeColor GetTagColor(ProfilerTag tag);
}

#endif // IYF_THREAD_PROFILER_SETTINGS_HPP

#if defined IYF_THREAD_PROFILER_IMPLEMENTATION && !defined IYF_THREAD_PROFILER_IMPLEMENTATION_INCLUDED

namespace iyft {
const char* GetTagName(ProfilerTag tag) {
    switch (tag) {
    case ProfilerTag::NoTag:
    case ProfilerTag::COUNT:
        return "Untagged";
    }
    
    IYF_ASSERT(0);
    return "ERROR-INVALID-VALUE";
}

ScopeColor GetTagColor(ProfilerTag tag) {
    switch (tag) {
    case ProfilerTag::NoTag:
    case ProfilerTag::COUNT:
        return ScopeColor(255, 255, 255, 255);
    }
    
    IYF_ASSERT(0);
    return ScopeColor(0, 0, 0, 255);
}
}

#endif // defined IYF_THREAD_PROFILER_IMPLEMENTATION && !defined IYF_THREAD_PROFILER_IMPLEMENTATION_INCLUDED
