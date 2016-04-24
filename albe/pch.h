/**
 * pch.h
 *
 *  Created on: Apr 25, 2015
 *      Author: Nick Pershin
 *      Company: AgileFusion Corp
 *
 *  Last edited on: May 06, 2015
 *      Editor: Nick Pershin
 *      Company: AgileFusion Corp
 *
 * Copyright (c) 2015 Agile Fusion. All rights reserved.
 */

#pragma once
#ifndef PCH_H_20150430_111345
#define PCH_H_20150430_111345

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>


#ifdef __cplusplus
extern "C" {
#endif



char STR_LIM[1024];  // one Kilo for sprintf()


//
// to see all predefines on current platform
// echo | g++ -dM -E -
// (predef.h from Boost lib rulez ;)
//
#if defined(WIN32) || defined(_WIN32) || defined(__TOS_WIN__) || defined(PREDEF_COMPILER_VISUALC)
    #define OS_WIN32           		1
    #define OS_MACOSX          		0
    #define OS_LINUX           		0
    #define DIR_DELIMITER           "\\"
#else
    #if defined(__MACOSX__) || (defined(__APPLE__) && defined(__MACH__))
        #define OS_WIN32       		0
        #define OS_MACOSX      		1
        #define OS_LINUX       		0
        #define DIR_DELIMITER       "/"
    #else
        #if defined(linux) || defined(__linux) || defined(__linux__)
            #define OS_WIN32   		0
            #define OS_MACOSX  		0
            #define OS_LINUX   		1
            #define DIR_DELIMITER   "/"
        #endif
    #endif
#endif



#define MAX_FILENAME 1024



#define ANSI_COLOR_RESET        "\x1b[0m"
#define ANSI_COLOR_BLACK        "\x1b[30m"
#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"
#define ANSI_COLOR_BLUE         "\x1b[34m"
#define ANSI_COLOR_MAGENTA      "\x1b[35m"
#define ANSI_COLOR_CYAN         "\x1b[36m"
#define ANSI_COLOR_WHITE        "\x1b[37m"
#define ANSI_COLOR_B_BLACK      "\x1b[30;1m"
#define ANSI_COLOR_B_RED        "\x1b[31;1m"
#define ANSI_COLOR_B_GREEN      "\x1b[32;1m"
#define ANSI_COLOR_B_YELLOW     "\x1b[33;1m"
#define ANSI_COLOR_B_BLUE       "\x1b[34;1m"
#define ANSI_COLOR_B_MAGENTA    "\x1b[35;1m"
#define ANSI_COLOR_B_CYAN       "\x1b[36;1m"
#define ANSI_COLOR_B_WHITE      "\x1b[37;1m"
#define ANSI_BG_BLACK           "\x1b[40m"
#define ANSI_BG_RED             "\x1b[41m"
#define ANSI_BG_GREEN           "\x1b[42m"
#define ANSI_BG_YELLOW          "\x1b[43m"
#define ANSI_BG_BLUE            "\x1b[44m"
#define ANSI_BG_MAGENTA         "\x1b[45m"
#define ANSI_BG_CYAN            "\x1b[46m"
#define ANSI_BG_WHITE           "\x1b[47m"



#ifdef __cplusplus
}
#endif

#endif /* #ifndef PCH_H_20150430_111345 */
