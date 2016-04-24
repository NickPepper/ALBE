// Copyright (c) 2015 Agile Fusion. All rights reserved.

/// @file unarch.cc
/// @author Nick Pershin

#include <stdlib.h>
#include <string.h>

#include "unarch.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
char* Reverse(const char* s) 
{
    size_t len = strlen(s);
    char* reversed = static_cast<char*>(malloc(len + 1));
    for (int i = len - 1; i >= 0; --i) {
        reversed[len - i - 1] = s[i];
    }
    reversed[len] = 0;
    return reversed;
}
*/

const char* SaySomething() 
{
    const int NSIDES = 6;
    const char* answer[NSIDES] = { "You touched the shared library 'libdumb.so', method SaySomething().\n\t\tМолодец. Хочешь, теперь либа потрогает тебя?", "ЗАЧЕМ ТЫ МЕНЯ ПОТРОГАЛ?", "МЕНЯ ЗОВУТ SaySomething(), НО Я ТЕБЕ НИЧЕГО НЕ СКАЖУ", "ЛУЧШЕ ПОТРОГАЙ МЕНЯ ЗАВТРА", "НЕ БОЛИТ ГОЛОВА У ДЯТЛА", "ЧТО Я ТЕБЕ СДЕЛАЛА?! Я всего лишь обычная тихая shared library", };
    return answer[rand() % NSIDES];
}


#ifdef __cplusplus
}
#endif
