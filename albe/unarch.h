// Copyright (c) 2015 Agile Fusion. All rights reserved.

/// @file unarch.h
/// @author Nick Pershin

#pragma once
#ifndef UNARCH_H_20150506_122227
#define UNARCH_H_20150506_122227


#ifdef __cplusplus
extern "C" {
#endif

// typedef char* (*TYPE_unarch)(const char*);
// extern "C" char* Reverse(const char*);

typedef char* (*TYPE_unarch)(void);
const char* SaySomething();


#ifdef __cplusplus
}
#endif

#endif /* #ifndef UNARCH_H_20150506_122227 */
