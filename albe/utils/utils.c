/*
 * utils.c
 *
 *  Created on: Apr 14, 2015
 *      Author: Nick Pershin
 *
 *  Last edited on: Apr 24, 2016
 *      Editor: Nick Pershin
 *
 */

#include "utils.h"


static const bool DEBUG = 0;// show debug text in console?


//---------------------------------- Common utils ---------------------------------------

int readLeInt(FILE* file) {
    int i;
    fread(&i, sizeof(int), 1, file);
    return i;
}


short readLeShort(FILE* file) {
    short s;
    fread(&s, sizeof(short), 1, file);
    return s;
}


void* L_Malloc(unsigned long len, void* pContext) {
    return malloc(len);
}


void L_Free(void* pMemory, void* pContext) {
    free(pMemory);
}


int makeSystemCall(const char* cmdname, char *args[], const char* dirpath)
{
    int status = 0;

    if(OS_MACOSX || OS_LINUX)
    {
        pid_t child_pid;

        // print error message if fork() fails
        if ( (child_pid = fork()) < 0 )
        {
            perror("fork failure");
            exit(EXIT_FAILURE);
            //status = -1;
        }

        // fork() == 0 for child process
        if (child_pid == 0)
        {
            execvp(cmdname, args);

            perror("execl() failure!\n\n");
            sprintf(STR_LIM, "\tThis print is after execl() and should not have been executed if execl were successful!\n\n");
            puts(STR_LIM);
            _exit(EXIT_FAILURE);
            // The reason for using _exit instead of exit is to avoid flushing fully buffered streams such as stdout.
            // The buffers of these streams probably contain data that was copied from the parent process by the fork,
            // data that will be output eventually by the parent process.
            // Calling exit in the child would output the data twice.
        }
        else  
        {
            // parent process
            wait(&status); // can use wait(NULL) since exit status from child is not used
        }

    } 
    else if(OS_WIN32) 
    {    
        char cmd[MAX_FILENAME];
        strcpy(cmd, cmdname);
        strcat(cmd, " ./");
        strcat(cmd, dirpath);
        // system() is not secure, but it seems there is no better way under Windows
        // it will simply report if the subdirectory already exists
        status = system(cmd);
    } 
    else 
    {
        DEBUG && ({
            sprintf(STR_LIM, "ERROR in makeSystemCall(): unsupported operating system! (supported are MAC OS X, Linux and WIN32)");
            puts(STR_LIM);
        });
        status = -1;
    }

    return status;
}




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
 * ATTENTION: caller must free() **p and **f after usage !
 */
void splitFullpathIntoPathAndFile(char *pf, char **p, char **f, char *delimiter) 
{
    // find the last delimiter's occurence
    char *z;
    for (z = pf+strlen(pf); z >= pf; z--) {
        if (*z == delimiter[0])
            break;
    }

    if (z >= pf) {
        // there is a delimiter: construct separate fragments for path and filename
        DEBUG && ({
            sprintf(STR_LIM, "\t--> %li\n", z-pf);
            puts(STR_LIM);
        });

        *p = malloc(z-pf+1);
        strncpy(*p, pf, z-pf);
        (*p)[z-pf] = '\0';
        *f = malloc(strlen(z));
        strcpy(*f, z+1);
    
    } else {
        // there is no delimiter: the entire string must be a filename
        *p = NULL;
        *f = malloc(strlen(pf)+1);
        strcpy(*f, pf);
    }
}




//----------------------------------- Base64 ---------------------------------------------

static const unsigned char
    m_arrBase64Decode[256] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,
            0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x02,
            0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
            0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
            0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1A,
            0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
            0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
            0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


unsigned long decodeBase64(const char *pSrcBuffer, unsigned char *pResultBuffer, unsigned long resultBufferSize)
{
    unsigned long dstIndex  = 0;
    unsigned char curByte   = 0;
    int shift               = 8;

    while (1) {
        unsigned char srcByte = *(pSrcBuffer++);
        if (srcByte == 0) {
            break;
        }

        // Convert the character into a six-bit value.
        // Illegal values in the Decode table return 0xFF.
        {
            unsigned char byte = m_arrBase64Decode[srcByte];
            if (byte < 0xFF) {
                // Shift the six-bit value and add to current byte.
                if ((shift -= 6) <= 0) {
                    if (dstIndex < resultBufferSize) {
                        pResultBuffer[dstIndex] = curByte | (byte >> -shift);
                    }
                    dstIndex++;
                    shift += 8;
                    curByte = 0;
                }
                curByte += (byte << shift);
            } else if (srcByte == '=') {
                break;
            }
        }
    }

    return dstIndex;
}


unsigned char* base64Decode(const char *data, unsigned long *outSize)
{
    unsigned long resultSize    = ((strlen(data) * 3) >> 2) + 2;
    unsigned char *result       = malloc(resultSize);// but where is free() ??
    unsigned long size          = decodeBase64(data, result, resultSize);

    if (outSize) {
        *outSize = size;
    }

    return result;
}




//----------------------------------- Hashmap --------------------------------------------

typedef struct Entry Entry;

struct Entry {
    void* key;
    int hash;
    void* value;
    Entry* next;
};

struct Hashmap {
    Entry** buckets;
    size_t bucketCount;
    int (*hash)(void* key);
    bool (*equals)(void* keyA, void* keyB);
    mutex_t lock; 
    size_t size;
};


Hashmap* hashmapCreate(size_t initialCapacity, int (*hash)(void* key), bool (*equals)(void* keyA, void* keyB)) 
{
    assert(hash != NULL);
    assert(equals != NULL);
    
    Hashmap* map = malloc(sizeof(Hashmap));
    if (map == NULL) {
        return NULL;
    }
    
    // 0.75 load factor
    size_t minimumBucketCount = (initialCapacity << 2) / 3;
    map->bucketCount = 1;
    while (map->bucketCount <= minimumBucketCount) {
        // Bucket count must be power of 2
        map->bucketCount <<= 1; 
    }

    map->buckets = calloc(map->bucketCount, sizeof(Entry*));
    if (map->buckets == NULL) {
        free(map);
        return NULL;
    }
    
    map->size = 0;

    map->hash = hash;
    map->equals = equals;
    
    mutex_init(&map->lock);
    
    return map;
}


/**
 * Hashes the given key.
 */
static inline int hashKey(Hashmap* map, void* key) 
{
    int h = map->hash(key);

    // We apply this secondary hashing discovered by Doug Lea to defend
    // against bad hashes.
    h += ~(h << 9);
    h ^= (((unsigned int) h) >> 14);
    h += (h << 4);
    h ^= (((unsigned int) h) >> 10);
       
    return h;
}


size_t hashmapSize(Hashmap* map) 
{
    return map->size;
}


static inline size_t calculateIndex(size_t bucketCount, int hash) 
{
    return ((size_t) hash) & (bucketCount - 1);
}


static void expandIfNecessary(Hashmap* map) 
{
    // If the load factor exceeds 0.75...
    if (map->size > ((map->bucketCount * 3) >> 2)) {
        // Start off with a 0.33 load factor.
        size_t newBucketCount = map->bucketCount << 1;
        Entry** newBuckets = calloc(newBucketCount, sizeof(Entry*));
        if (newBuckets == NULL) {
            // Abort expansion.
            return;
        }
        
        // Move over existing entries.
        size_t i;
        for (i = 0; i < map->bucketCount; i++) {
            Entry* entry = map->buckets[i];
            while (entry != NULL) {
                Entry* next = entry->next;
                size_t index = calculateIndex(newBucketCount, entry->hash);
                entry->next = newBuckets[index];
                newBuckets[index] = entry;
                entry = next;
            }
        }

        // Copy over internals.
        free(map->buckets);
        map->buckets = newBuckets;
        map->bucketCount = newBucketCount;
    }
}


void hashmapLock(Hashmap* map) 
{
    mutex_lock(&map->lock);
}


void hashmapUnlock(Hashmap* map) 
{
    mutex_unlock(&map->lock);
}


void hashmapFree(Hashmap* map) 
{
    size_t i;
    for (i = 0; i < map->bucketCount; i++) {
        Entry* entry = map->buckets[i];
        while (entry != NULL) {
            Entry* next = entry->next;
            free(entry);
            entry = next;
        }
    }
    if ( map->buckets )
        free(map->buckets);

    mutex_destroy(&map->lock);
    if ( map )
        free(map);
}


int hashmapHash(void* key, size_t keySize) 
{
    int h = keySize;
    char* data = (char*) key;
    size_t i;
    for (i = 0; i < keySize; i++) {
        h = h * 31 + *data;
        data++;
    }
    return h;
}


static Entry* createEntry(void* key, int hash, void* value) 
{
    Entry* entry = malloc(sizeof(Entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->key = key;
    entry->hash = hash;
    entry->value = value;
    entry->next = NULL;
    return entry;
}


static inline bool equalKeys(void* keyA, int hashA, void* keyB, int hashB, bool (*equals)(void*, void*)) 
{
    if (keyA == keyB) {
        return true;
    }
    if (hashA != hashB) {
        return false;
    }
    return equals(keyA, keyB);
}


void* hashmapPut(Hashmap* map, void* key, void* value) 
{
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);

    Entry** p = &(map->buckets[index]);
    while (true) {
        Entry* current = *p;

        // Add a new entry.
        if (current == NULL) {
            *p = createEntry(key, hash, value);
            if (*p == NULL) {
                errno = ENOMEM;
                return NULL;
            }
            map->size++;
            expandIfNecessary(map);
            return NULL;
        }

        // Replace existing entry.
        if (equalKeys(current->key, current->hash, key, hash, map->equals)) {
            void* oldValue = current->value;
            current->value = value;
            return oldValue;
        }

        // Move to next entry.
        p = &current->next;
    }
}


void* hashmapGet(Hashmap* map, void* key) 
{
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);

    Entry* entry = map->buckets[index];
    while (entry != NULL) {
        if (equalKeys(entry->key, entry->hash, key, hash, map->equals)) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL;
}


bool hashmapContainsKey(Hashmap* map, void* key) 
{
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);

    Entry* entry = map->buckets[index];
    while (entry != NULL) {
        if (equalKeys(entry->key, entry->hash, key, hash, map->equals)) {
            return true;
        }
        entry = entry->next;
    }

    return false;
}


void* hashmapMemoize(Hashmap* map, void* key, void* (*initialValue)(void* key, void* context), void* context) 
{
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);

    Entry** p = &(map->buckets[index]);
    while (true) {
        Entry* current = *p;

        // Add a new entry.
        if (current == NULL) {
            *p = createEntry(key, hash, NULL);
            if (*p == NULL) {
                errno = ENOMEM;
                return NULL;
            }
            void* value = initialValue(key, context);
            (*p)->value = value;
            map->size++;
            expandIfNecessary(map);
            return value;
        }

        // Return existing value.
        if (equalKeys(current->key, current->hash, key, hash, map->equals)) {
            return current->value;
        }

        // Move to next entry.
        p = &current->next;
    }
}


void* hashmapRemove(Hashmap* map, void* key) 
{
    int hash = hashKey(map, key);
    size_t index = calculateIndex(map->bucketCount, hash);

    // Pointer to the current entry.
    Entry** p = &(map->buckets[index]);
    Entry* current;
    while ((current = *p) != NULL) {
        if (equalKeys(current->key, current->hash, key, hash, map->equals)) {
            void* value = current->value;
            *p = current->next;
            free(current);
            map->size--;
            return value;
        }

        p = &current->next;
    }

    return NULL;
}


void hashmapForEach(Hashmap* map, bool (*callback)(void* key, void* value, void* context), void* context) 
{
    size_t i;
    for (i = 0; i < map->bucketCount; i++) {
        Entry* entry = map->buckets[i];
        while (entry != NULL) {
            if (!callback(entry->key, entry->value, context)) {
                return;
            }
            entry = entry->next;
        }
    }
}


size_t hashmapCurrentCapacity(Hashmap* map) 
{
    size_t bucketCount = map->bucketCount;
    return (bucketCount * 3) >> 2;
}


size_t hashmapCountCollisions(Hashmap* map) 
{
    size_t collisions = 0;
    size_t i;
    for (i = 0; i < map->bucketCount; i++) {
        Entry* entry = map->buckets[i];
        while (entry != NULL) {
            if (entry->next != NULL) {
                collisions++;
            }
            entry = entry->next;
        }
    }
    return collisions;
}


int hashmapIntHash(void* key) 
{
    // Return the key value itself.
    return *((int*) key);
}


bool hashmapIntEquals(void* keyA, void* keyB) 
{
    int a = *((int*) keyA);
    int b = *((int*) keyB);
    return a == b;
}

