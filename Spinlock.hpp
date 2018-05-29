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

/// \file Spinlock.hpp Contains an an atomic_flag based spinlock.

#ifndef IYFT_SPINLOCK_HPP
#define IYFT_SPINLOCK_HPP

#include <atomic>

namespace iyft {
/// \brief An atomic_flag based spinlock.
///
/// Lower latency than std::mutex because it avoids system calls. However, it is a form
/// of busy waiting and should only be used for very short operations because you'll be
/// wasting your CPU's time otherwise.
class Spinlock {
public:
    /// \brief Locks the spinlock.
    void lock() {
        while (spinlock.test_and_set(std::memory_order_acquire)) {;}
    }
    
    /// \brief Unlocks the spinlock.
    void unlock() {
        spinlock.clear(std::memory_order_release);
    }
private:
    /// \brief The atomic_flag that is manipulated by the functions of this class.
    std::atomic_flag spinlock = ATOMIC_FLAG_INIT;
};

}

#endif // IYFT_SPINLOCK_HPP
