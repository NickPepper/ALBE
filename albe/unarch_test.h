/**
 * unarch_test.h
 *
 *  Created on: Apr 14, 2015
 *      Author: Nick Pershin
 *      Company: AgileFusion Corp
 *
 *  Last edited on: Apr 26, 2015
 *      Editor: Nick Pershin
 *      Company: AgileFusion Corp
 *
 *
 *  Based on epub_content_loader.c from AFDRMLibTurbo
 *  Created on: Apr 14, 2011
 *      Authors: Anton Antonov & Sergey Bicarte
 *		Company: AgileFusion Corp
 *
 * Copyright (c) 2015 Agile Fusion. All rights reserved.
 */

#pragma once
#ifndef UNARCH_TEST_H_20150430_111307
#define UNARCH_TEST_H_20150430_111307

#ifdef __cplusplus
extern "C" {
#endif



//-------------------------------- Data --------------------------------------

typedef struct S_ZIPENTRY {
    char* name;
    char* extra;
    char* comment;
    unsigned int dostime;
    unsigned int crc;
    unsigned int csize;
    unsigned int size;
    unsigned int offset;
    unsigned int hash;
    unsigned short method;
    unsigned short nameLen;
    unsigned short extraLen;
    unsigned short commentLen;
} ZIPENTRY;

static FILE* m_fd = NULL;
static const char* m_fname;
static ZIPENTRY* m_arrEntries = NULL;
static Hashmap* m_mpMap = NULL;
static int m_nEntriesCount = 0;


//#define READ_SIZE             8192
//#define BLOCK_SIZE            4096
#define COMMENTS_BLOCK_SIZE     65536


//--------------------------- Compression method ---------------------------------
#define METHOD_STORED   0
#define METHOD_DEFLATED 8


//------------------------------ Error codes ------------------------------------

//#define BASE_ERROR_CODE                              503000
#define UNZIP_ERR_CENTRAL_DIRECTORY_NOT_FOUND        503001
#define UNZIP_ERR_WRONG_CENTRAL_DIRECTORY_SIGNATURE  503002
#define UNZIP_ERR_WRONG_LOCAL_HEADER_SIGNATURE       503003
#define UNZIP_ERR_COMPRESSION_METHOD_MISMATCH        503004
#define UNZIP_ERR_UNCOMPRESSION_FAILED               503005
//#define EPUB_ERR_FILE_IS_NOT_OPENED                 503006
//#define EPUB_ERR_INVALID_HASH                       503007



//--------------------------- Header signatures ---------------------------------

// "PK\003\004"
#define LOCSIG 0x04034b50L
// "PK\007\008"
#define EXTSIG 0x08074b50L
// "PK\001\002"
#define CENSIG 0x02014b50L
// "PK\005\006"
#define ENDSIG 0x06054b50L



//--------------- Header sizes in bytes (including signatures) ------------------

// LOC header size
#define LOCHDR 30
// EXT header size
#define EXTHDR 16
// CEN header size
#define CENHDR 46
// END header size
#define ENDHDR 22



//----------- End of central directory (END) header field offsets ---------------

// number of entries on this disk
#define ENDSUB 8
// total number of entries
#define ENDTOT 10
// central directory size in bytes
#define ENDSIZ 12
// offset of first CEN header
#define ENDOFF 16
// zip file comment length
#define ENDCOM 20

#define ENDNRD 4




#ifdef __cplusplus
}
#endif

#endif /* #ifndef UNARCH_TEST_H_20150430_111307 */ 
