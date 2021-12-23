//---------------------------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright(c) 2021 David "Rez" Graham
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

//---------------------------------------------------------------------------------------------------------------------
// This file allows you to configure BleachLua in a number of different ways.  Read the comments below for details 
// on what each setting does.  Note that several settings shouldn't be modified since they are specific to my internal 
// engine.  They are noted below.
//---------------------------------------------------------------------------------------------------------------------

// Divide this by ten for the true major version.  For eample, 52 means Lua 5.2.  We ignore build versions since 
// they are guaranteed to be compatible.
#define BLEACHLUA_CORE_VERSION 53  // valid values are 52 and 53

// If set to 1, this will enable BleachLua unit tests to be called.  You have to call them manually (see LuaError.h 
// for details).
#define BLEACHLUA_ENABLE_UNIT_TESTS 0

// Set this to 1 to use EASTL containers & algorithms rather than the std.  You will need to deal with setting up 
// the EASTL include paths yourself, this won't do that for you, it just includes and uses them as normal.
#define BLEACHLUA_USE_EASTL 0

// This shouldn't be modified.  Several places in this code use engine features from my internal Bleach engine and 
// this serves as a simple way for me to conditionally compile that stuff in.  The idea is to keep the delta from 
// this repository and my internal BleachLua code as small as possible.
#define BLEACHLUA_USING_BLEACH_ENGINE 0

// In debug mode, extra safety checks and and error checking are enabled.  Debug is defined differently for the 
// Bleach engine (there are several "debug" modes).  The non-Bleach engine logic should work in the simple case 
// and follows the C++ standards.  Feel free to change this logic for your own engine.
// 
// There are two flags here: 
//      -BLEACHLUA_DEBUG_CHECKS:    If set, this means extra checks are done, asserts are thrown, and errors are 
//                                  logged.
//      -BLEACHLUA_DEBUG_MODE:      This represents true debug mode (i.e. the equivilance of _DEBUG being defined in 
//                                  MSVC).  This is only used for things you might want to look at with a debugger.
#if BLEACHLUA_USING_BLEACH_ENGINE
    #include <BleachUtils/Debug/Debugger.h>
    #define BLEACHLUA_DEBUG_CHECKS _LOGS_ENABLED 
    #ifdef _DEBUG
        #define BLEACHLUA_DEBUG_MODE 1
    #else
        #define BLEACHLUA_DEBUG_MODE 0
    #endif
#else  // not using bleach engine
    // If we're not using the Bleach engine, NDEBUG is the only thing we can rely on since it's the only standards-
    // compliant way to tell the difference release builds and non-release builds.
    #ifdef NDEBUG
        #define BLEACHLUA_DEBUG_CHECKS 0
        #define BLEACHLUA_DEBUG_MODE 0
    #else
        #define BLEACHLUA_DEBUG_CHECKS 1
        #define BLEACHLUA_DEBUG_MODE 1
    #endif
#endif

// Macros to wrap new & delete.  There is only one place BleachLua does heap allocation, which for the ref counts in 
// LuaVar.  In the Bleach engine, I use a memory leak detector, so I have a specific macro that replaces new/delete.
// You can find it here: https://github.com/bleachkitty/BleachLeakDetector
// 
// If you're using the Bleach Leak Detector, feel free to delete the conditional here.  You can also replace the 
// #else clause if your code has its own system, or if you want to pool them or something.
#if BLEACHLUA_USING_BLEACH_ENGINE
    #define BLEACHLUA_NEW BLEACH_NEW
    #define BLEACHLUA_DELETE BLEACH_DELETE
#else
    #define BLEACHLUA_NEW new
    #define BLEACHLUA_DELETE delete
#endif

// This shouldn't be modified.  The Bleach engine uses memory pooling for LuaVar's ref count.  I have an extra flag 
// for it because I need to be able to turn off memory pooling in case I need to debug a memory pool issue.
#if BLEACHLUA_USING_BLEACH_ENGINE
    #define BLEACHLUA_USE_MEMORY_POOLS 1
#else
    #define BLEACHLUA_USE_MEMORY_POOLS 0
#endif
