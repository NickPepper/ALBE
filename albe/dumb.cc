/// @file dumb.cc
/// @author Nick Pershin

#include <stdlib.h>
#include <stdio.h>

#include "dumb.h"

#ifdef __cplusplus
extern "C" {
#endif


const char* SaySomething() 
{
    const int NSIDES = 6;
    const char* answer[NSIDES] = { "You touched the shared library 'libdumb.so', method SaySomething().\n\t\tМолодец. Хочешь, теперь либа потрогает тебя?", "ЗАЧЕМ ТЫ МЕНЯ ПОТРОГАЛ?", "МЕНЯ ЗОВУТ SaySomething(), НО Я ТЕБЕ НИЧЕГО НЕ СКАЖУ", "ЛУЧШЕ ПОТРОГАЙ МЕНЯ ЗАВТРА", "НЕ БОЛИТ ГОЛОВА У ДЯТЛА", "ЧТО Я ТЕБЕ СДЕЛАЛА?! Я всего лишь обычная тихая shared library", };
    return answer[rand() % NSIDES];
}


#ifdef __cplusplus
}
#endif
