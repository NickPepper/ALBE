/// @file albe.h
/// @author Nick Pershin

#pragma once
#ifndef ALBE_H_20150428_125110
#define ALBE_H_20150428_125110

#define __STDC_LIMIT_MACROS

#ifndef INT32_MAX
    #define INT32_MAX (0x7FFFFFFF)
#endif

#ifdef WIN32
    #undef min
    #undef max
    #undef PostMessage
    #pragma warning(disable : 4355)// allow 'this' in initializer list
#endif


#if defined(NACL_SDK_DEBUG)
    #define CONFIG_NAME "Debug"
#else
    #define CONFIG_NAME "Release"
#endif


#if defined __arm__
    #define NACL_ARCH "arm"
#elif defined __i686__
    #define NACL_ARCH "x86_32"
#elif defined __x86_64__
    #define NACL_ARCH "x86_64"
#else
    #error "Unknown architecture"
#endif


namespace {
    const char* const kLoadPrefix            = "ld";
    const char* const kLoadExternalPrefix    = "ld_ext";
    const char* const kSavePrefix            = "sv";
    const char* const kSaveToExterntalPrefix = "sv_ext";
    const char* const kDeletePrefix          = "rm";
    const char* const kListPrefix            = "ls";
    const char* const kMakeDirPrefix         = "md";

    const char* const kSharedLibDlopen       = "dlopen";
    const char* const kSharedLibHttpMount    = "httpmount";
}


#endif /* ALBE_H_20150428_125110 */
