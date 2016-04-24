/**
 * utils.h
 *
 *  Created on: Apr 14, 2015
 *      Author: Nick Pershin
 *
 *  Last edited on: Apr 24, 2016
 *      Editor: Nick Pershin
 */

#ifndef UTILS_H_20150430_111419
#define UTILS_H_20150430_111419

#include "../pch.h"

#define HAVE_PTHREADS
#include "threads.h"


#ifdef __cplusplus
extern "C" {
#endif



//---------------------------------- Common utils ---------------------------------------
int readLeInt(FILE* file);
short readLeShort(FILE* file);
void* L_Malloc(unsigned long len, void* pContext);
void L_Free(void* pMemory, void* pContext);
int makeSystemCall(const char* cmdname, char *args[], const char* dirpath);




//------------------------------- String operations --------------------------------------
/**
 * Splits the FULL-PATH (example: "~/folder/subfolder/file.file") into 
 * PATH (example: "~/folder/subfolder") and
 * FILE (example: "file.file") parts.
 *
 * @param pf - pointer to fullpath string
 * @param p - pointer to string where the PATH part will be stored
 * @param f - pointer to string where the FILE part will be stored
 * @param delimiter - is "/" on UNIX and "\\" on Windows
 *
 * ATTENTION: the caller must free() *p and *f after usage !
 */
void splitFullpathIntoPathAndFile(char *pf, char **p, char **f, char *delimiter);



//----------------------------------- Base64 ---------------------------------------------
unsigned char* base64Decode(const char* data, unsigned long* outSize);




//----------------------------------- Hashmap --------------------------------------------
/** A hash map. */
typedef struct Hashmap Hashmap;

/**
 * Creates a new hash map. Returns NULL if memory allocation fails.
 *
 * @param initialCapacity number of expected entries
 * @param hash function which hashes keys
 * @param equals function which compares keys for equality
 */
Hashmap* hashmapCreate(size_t initialCapacity, int (*hash)(void* key), bool (*equals)(void* keyA, void* keyB));

/**
 * Frees the hash map. Does not free the keys or values themselves.
 */
void hashmapFree(Hashmap* map);

/**
 * Hashes the memory pointed to by key with the given size. Useful for
 * implementing hash functions.
 */
int hashmapHash(void* key, size_t keySize);

/**
 * Puts value for the given key in the map. Returns pre-existing value if
 * any.
 *
 * If memory allocation fails, this function returns NULL, the map's size
 * does not increase, and errno is set to ENOMEM.
 */
void* hashmapPut(Hashmap* map, void* key, void* value);

/**
 * Gets a value from the map. Returns NULL if no entry for the given key is
 * found or if the value itself is NULL.
 */
void* hashmapGet(Hashmap* map, void* key);

/**
 * Returns true if the map contains an entry for the given key.
 */
bool hashmapContainsKey(Hashmap* map, void* key);

/**
 * Gets the value for a key. If a value is not found, this function gets a 
 * value and creates an entry using the given callback.
 *
 * If memory allocation fails, the callback is not called,
 * this function returns NULL, and errno is set to ENOMEM.
 */
void* hashmapMemoize(Hashmap* map, void* key, void* (*initialValue)(void* key, void* context), void* context);

/**
 * Removes an entry from the map. Returns the removed value or NULL if no
 * entry was present.
 */
void* hashmapRemove(Hashmap* map, void* key);

/**
 * Gets the number of entries in this map.
 */
size_t hashmapSize(Hashmap* map);

/**
 * Invokes the given callback on each entry in the map. Stops iterating if
 * the callback returns false.
 */
void hashmapForEach(Hashmap* map, bool (*callback)(void* key, void* value, void* context), void* context);

/**
 * Concurrency support.
 */

/**
 * Locks the hash map so only the current thread can access it.
 */
void hashmapLock(Hashmap* map);

/**
 * Unlocks the hash map so other threads can access it.
 */
void hashmapUnlock(Hashmap* map);

/**
 * Key utilities.
 */

/**
 * Hashes int keys. 'key' is a pointer to int.
 */
int hashmapIntHash(void* key);

/**
 * Compares two int keys for equality.
 */
bool hashmapIntEquals(void* keyA, void* keyB);

/**
 * For debugging.
 */

/**
 * Gets current capacity.
 */
size_t hashmapCurrentCapacity(Hashmap* map);

/**
 * Counts the number of entry collisions.
 */
size_t hashmapCountCollisions(Hashmap* map);




#ifdef __cplusplus
}
#endif

#endif /* #ifndef UTILS_H_20150430_111419 */
