/*
 * unarch_test.c
 *
 * Can open and unzip .EPUB, .DOCX, .FOLIO, .ZIP and other zipped archives
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
 *      Company: AgileFusion Corp
 *
 * Copyright (c) 2015 Agile Fusion. All rights reserved.
 *
 *
 * BUILD DEBUG:
 *      gcc -std=c99 -I../.. -Wall -g -o unarch_test unarch_test.c utils/utils.c -lz
 * or (on MAC OS X only)
 *      gcc -I../.. -Wall -g -o unarch_test unarch_test.c utils/utils.c libs/libz.a
 *
 * BUILD RELEASE:
 *      gcc -std=c99 -I../.. -Wall -O3 -o unarch_test unarch_test.c utils/utils.c -lz
 * or (on MAC OS X only)
 *      gcc -I../.. -Wall -O3 -o unarch_test unarch_test.c utils/utils.c libs/libz.a
 *
 *
 * RUN EXAMPLE:
 * (with relative PATHs)
 *      ./unarch_test input_data/Организ.\ пр-ва\ на\ предпр.\ ЖКХ\ РП.docx output_data
 * or (for absolute PATHs) something like:
 *      ./unarch_test ~/work/epub3-editor/albe/c_layer/input_data/Организ.\ пр-ва\ на\ предпр.\ ЖКХ\ РП.docx ~/work/epub3-editor/albe/c_layer/output_data
 *
 *
 * FAKE .docx file (XML inside, not zipped)
 *      ./unarch_test input_data/fake.docx output_data
 *
 * LARGE CONTENT (takes a long time to process):
 *      ./unarch_test input_data/Adobe_RMSDK_20140610_Beta.zip output_data
 * 
 */


#include <time.h>
#include "libs/zlib.h" // #include <zlib.h> ?
#include "utils/utils.h"
#include "unarch_test.h"


// show debug text in console?
static const bool DEBUG = 0;


//--------------------------- HEART SECTION -------------------------------


static int hashFn(void* pKey) 
{
    ZIPENTRY* pData = (ZIPENTRY*) pKey;
    if (pData->hash == 0) 
    {
        pData->hash = hashmapHash((void*) pData->name, strlen(pData->name));
        DEBUG && ({
            sprintf(STR_LIM, "hashFn() :: hashmapHash name: %s, hash: %u", pData->name, pData->hash);
            puts(STR_LIM);
        });
    }
    return pData->hash;
}


static bool equalsFn(void* keyA, void* keyB) 
{
    ZIPENTRY* pDataA = (ZIPENTRY*) keyA;
    ZIPENTRY* pDataB = (ZIPENTRY*) keyB;
    DEBUG && ({
        sprintf(STR_LIM, "equalsFn a: %s, b: %s", pDataA->name, pDataB->name);
        puts(STR_LIM);
    });
    return strcmp(pDataA->name, pDataB->name) == 0;
}


/**
 * This function is declared as not 'void' but as 'bool'
 * just to allow calls within logical expressions like:
 * DEBUG && cleanup(); etc.
 */
static bool cleanup()
{
    if (m_arrEntries)
    {
        for (int i = 0; i < m_nEntriesCount; i++)
        {
            if (m_arrEntries[i].name)
            {
                DEBUG && ({
                    sprintf(STR_LIM, "cleanup() : name = %s", m_arrEntries[i].name);
                    puts(STR_LIM);
                });
                free(m_arrEntries[i].name);
            }
            
            if (m_arrEntries[i].extra)
            {
                DEBUG && ({
                    sprintf(STR_LIM, "cleanup() : extra = %s", m_arrEntries[i].extra);
                    puts(STR_LIM);
                });
                free(m_arrEntries[i].extra);
            }
            
            if (m_arrEntries[i].comment)
            {
                DEBUG && ({
                    sprintf(STR_LIM, "cleanup() : comment = %s", m_arrEntries[i].comment);
                    puts(STR_LIM);
                });
                free(m_arrEntries[i].comment);
            }
        }
        
        free(m_arrEntries);
        m_arrEntries = NULL;
        m_nEntriesCount = 0;
    }

    if (m_fd) {
        fclose(m_fd);
        m_fd = NULL;
    }

    if (m_mpMap){
        hashmapFree(m_mpMap);
        m_mpMap = NULL;
    }

    return 0;
}


/**
 * This function is declared as not 'void' but as 'bool'
 * just to allow calls within logical expressions like:
 * DEBUG && printZipEntryInfo(&m_arrEntries[i]); etc.
 */
static bool printZipEntryInfo(ZIPENTRY* entry)
{
    sprintf(STR_LIM, "\nname: %s, nameLen: %u", entry->name, entry->nameLen);
    puts(STR_LIM);

    sprintf(STR_LIM, "extra: %s, extraLen: %u", entry->extra, entry->extraLen);
    puts(STR_LIM);

    sprintf(STR_LIM, "comment: %s, commentLen: %u", entry->comment, entry->commentLen);
    puts(STR_LIM);

    sprintf(STR_LIM, "csize: %u, size: %u", entry->csize, entry->size);
    puts(STR_LIM);

    sprintf(STR_LIM, "method: %u", entry->method);
    puts(STR_LIM);

    sprintf(STR_LIM, "dostime: %u", entry->dostime);
    puts(STR_LIM);

    sprintf(STR_LIM, "crc: %u", entry->crc);
    puts(STR_LIM);

    sprintf(STR_LIM, "offset: %u", entry->offset);
    puts(STR_LIM);

    sprintf(STR_LIM, "hash: %d", entry->hash);
    puts(STR_LIM);

    return 0;
}



static int uncompress2(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen) 
{
    z_stream stream;
    int err = 0;

    stream.next_in = (Bytef*) source;
    stream.avail_in = (uInt) sourceLen;
    // check for source > 64K on 16-bit machine:
    if ( (uLong) stream.avail_in != sourceLen ) {
        err = Z_BUF_ERROR;
        goto error;
    }

    stream.next_out = dest;
    stream.avail_out = (uInt) *destLen;
    if ( (uLong) stream.avail_out != *destLen ) {
        err = Z_BUF_ERROR;
        goto error;
    }

    stream.zalloc = (alloc_func) 0;
    stream.zfree = (free_func) 0;

    err = inflateInit2(&stream, -MAX_WBITS);
    if (err != Z_OK) {
        goto error;
    }

    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        if ( err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0) ) {
            err = Z_DATA_ERROR;
            goto error;
        }
        goto error;
    }
    *destLen = stream.total_out;

    err = inflateEnd(&stream);

error:

    return err;
}



static ZIPENTRY* getZipEntry(const char* name) 
{
    ZIPENTRY oKey   = {0};
    oKey.hash       = 0;
    oKey.name       = (char*)name;

    return (ZIPENTRY*) hashmapGet(m_mpMap, &oKey);
}



static int openZipArchive(const char* filePath)
{
    long size           = 0;
    long pos            = 0;
    long top            = 0;
    int i               = 0;
    int count           = 0;
    int retval          = 0;
    int centralOffset   = 0;

    m_fname = filePath;
    m_fd = fopen(m_fname, "rb");

    if (m_fd == NULL)
    {
        DEBUG && ({
            sprintf(STR_LIM, "openZipArchive() : fopen failed!");
            puts(STR_LIM);
        });

        retval = errno;
        goto error;
    }

    fseek(m_fd, 0, SEEK_END);
    size = ftell(m_fd);
    fseek(m_fd, 0, SEEK_SET);

    pos = size - ENDHDR;
    top = pos - COMMENTS_BLOCK_SIZE;
    if (top < 0) {
        top = 0;
    }

    //read m_arrEntries
    do {
        if (pos < top)
        {
            DEBUG && ({
                sprintf(STR_LIM, "central directory not found, probably not a zip file: %s", m_fname);
                puts(STR_LIM);
            });

            retval = UNZIP_ERR_CENTRAL_DIRECTORY_NOT_FOUND;
            goto error;
        }

        fseek(m_fd, pos--, SEEK_SET);

    } while (readLeInt(m_fd) != ENDSIG);

    pos = ftell(m_fd);
    
    if (fseek(m_fd, pos + (ENDTOT - ENDNRD), SEEK_SET))
    {
        DEBUG && ({
            sprintf(STR_LIM, "eof file: %s", m_fname);
            puts(STR_LIM);
        });

        retval = errno;
        goto error;
    }

    count = readLeShort(m_fd);


    //load entries
    m_arrEntries = malloc(count * sizeof(ZIPENTRY));
    memset(m_arrEntries, 0, count * sizeof(ZIPENTRY));
    m_nEntriesCount = count;

    m_mpMap = hashmapCreate(count, hashFn, equalsFn);

    pos = ftell(m_fd);
    if (fseek(m_fd, pos + (ENDOFF - ENDSIZ), SEEK_SET)) 
    {
        DEBUG && ({
            sprintf(STR_LIM, "eof file: %s", m_fname);
            puts(STR_LIM);
        });

        retval = errno;
        goto error;
    }

    centralOffset = readLeInt(m_fd);

    if (fseek(m_fd, centralOffset, SEEK_SET)) 
    {
        DEBUG && ({
            sprintf(STR_LIM, "central directory not found, probably not a zip file: %s", m_fname);
            puts(STR_LIM);
        });

        retval = UNZIP_ERR_CENTRAL_DIRECTORY_NOT_FOUND;
        goto error;
    }

    for (i = 0; i < count; i++) 
    {
        if (readLeInt(m_fd) != CENSIG) 
        {
            DEBUG && ({
                sprintf(STR_LIM, "Wrong Central Directory signature: %s", m_fname);
                puts(STR_LIM);
            });

            retval = UNZIP_ERR_WRONG_CENTRAL_DIRECTORY_SIGNATURE;
            goto error;
        }

        pos = ftell(m_fd);
        fseek(m_fd, pos + 6, SEEK_SET); //skip 6 bytes

        m_arrEntries[i].method      = readLeShort(m_fd);
        m_arrEntries[i].dostime     = readLeInt(m_fd);
        m_arrEntries[i].crc         = readLeInt(m_fd);
        m_arrEntries[i].csize       = readLeInt(m_fd);
        m_arrEntries[i].size        = readLeInt(m_fd);
        m_arrEntries[i].nameLen     = readLeShort(m_fd);
        m_arrEntries[i].extraLen    = readLeShort(m_fd);
        m_arrEntries[i].commentLen  = readLeShort(m_fd);

        pos = ftell(m_fd);
        fseek(m_fd, pos + 8, SEEK_SET); //skip 8 bytes

        m_arrEntries[i].offset = readLeInt(m_fd);

        if (m_arrEntries[i].nameLen > 0) {
            m_arrEntries[i].name = malloc(m_arrEntries[i].nameLen + 1);
            memset(m_arrEntries[i].name,0,m_arrEntries[i].nameLen + 1);
            fread(m_arrEntries[i].name, sizeof(char), m_arrEntries[i].nameLen, m_fd);
        }
        
        if (m_arrEntries[i].extraLen > 0) {
            m_arrEntries[i].extra = malloc(m_arrEntries[i].extraLen);
            fread(m_arrEntries[i].extra, sizeof(char), m_arrEntries[i].extraLen, m_fd);
        }

        if (m_arrEntries[i].commentLen > 0) {
            m_arrEntries[i].comment = malloc(m_arrEntries[i].commentLen + 1);
            memset(m_arrEntries[i].comment,0,m_arrEntries[i].commentLen + 1);
            fread(m_arrEntries[i].comment, sizeof(char), m_arrEntries[i].commentLen, m_fd);
        }

        DEBUG && printZipEntryInfo(&m_arrEntries[i]);

        //put to hash map for quick access
        hashmapPut(m_mpMap, m_arrEntries + i, m_arrEntries + i);
    }

error:

    return retval;
}



static unsigned char* getEntryData(const char* name,
                unsigned long* destLen,
                void* (*pfnAlloc)(unsigned long len, void* pContext),
                void (*pfnFree)(void* mem, void* pContext),
                void* pCallbackContext )
{
    int retval          = 0;
    int pos             = 0;
    int nameLen         = 0;
    int extraLen        = 0;
    unsigned char* dest = NULL;
    unsigned char* src  = NULL;


    pfnAlloc = pfnAlloc ? pfnAlloc : &L_Malloc;
    pfnFree = pfnFree ? pfnFree : &L_Free;

    DEBUG && ({
        sprintf(STR_LIM, "getEntryData %s", name);
        puts(STR_LIM);
    });

    if (!m_fd) 
    {
        DEBUG && ({
            sprintf(STR_LIM, "getEntryData returning NULL, file closed for %s", name);
            puts(STR_LIM);
        });
        dest = NULL;
        goto error;
    }

    hashmapLock(m_mpMap);

    ZIPENTRY* entry = getZipEntry(name);

    if (entry) 
    {
        DEBUG && ({
            sprintf(STR_LIM, "found entry csize: %d size: %d", entry->csize, entry->size);
            puts(STR_LIM);
        });

        src = pfnAlloc(entry->csize, pCallbackContext);
        if (destLen) {
            *destLen = entry->size;
        }

        fseek(m_fd, entry->offset, SEEK_SET);

        if (readLeInt(m_fd) != LOCSIG) 
        {
            DEBUG && ({
                sprintf(STR_LIM, "Wrong Local header signature: %s", m_fname);
                puts(STR_LIM);
            });

            retval = UNZIP_ERR_WRONG_LOCAL_HEADER_SIGNATURE;
            goto get_entry_error;
        }

        pos = ftell(m_fd);
        fseek(m_fd, pos + 4, SEEK_SET); //skip 4 bytes

        if (entry->method != readLeShort(m_fd)) 
        {
            DEBUG && ({
                sprintf(STR_LIM, "Compression method mismatch: %s", m_fname);
                puts(STR_LIM);
            });

            retval = UNZIP_ERR_COMPRESSION_METHOD_MISMATCH;
            goto get_entry_error;
        }

        pos = ftell(m_fd);
        fseek(m_fd, pos + 16, SEEK_SET); //skip 16 bytes

        nameLen = readLeShort(m_fd);
        extraLen = readLeShort(m_fd);

        pos = ftell(m_fd);
        fseek(m_fd, pos + (nameLen + extraLen), SEEK_SET); //skip bytes

        //read compressed bytes
        DEBUG && ({
            sprintf(STR_LIM, "read compressed bytes, method: %d", entry->method);
            puts(STR_LIM);
        });

        if (entry->method == METHOD_STORED) 
        {
            DEBUG && ({
                sprintf(STR_LIM, "YEP! entry->method == METHOD_STORED\n");
                puts(STR_LIM);
            });

            dest = pfnAlloc(entry->size, pCallbackContext);
            fread(dest, entry->size, sizeof(Bytef), m_fd);
        } 
        else 
        {
            DEBUG && ({
                sprintf(STR_LIM, "YEP! entry->method == METHOD_DEFLATED\n");
                puts(STR_LIM);
            });

            fread(src, entry->csize, sizeof(Bytef), m_fd);

            int unzipFactor = 1;
            int k = 10;
            while (k > 0) {
                *destLen = entry->size * unzipFactor;
                if (dest) {
                    pfnFree(dest, pCallbackContext);
                }
                dest = pfnAlloc(*destLen, pCallbackContext);

                if (uncompress2(dest, destLen, src, entry->csize)) 
                {
                    sprintf(STR_LIM, "\nuncompress2() failed to uncompress to dest %s (destLen = %lu)\n\n", dest, *destLen);
                    puts(STR_LIM);

                    k--;
                    unzipFactor *= 2;
                    if (dest) {
                        pfnFree(dest, pCallbackContext);
                        dest = NULL;
                    }

                } else {
                    // uncompress2() returns 0 if all was OK
                    break;
                }
            }

            if (k == 0) {
                DEBUG && ({
                    sprintf(STR_LIM, "failed to uncompress data %s", name);
                    puts(STR_LIM);
                });

                retval = UNZIP_ERR_UNCOMPRESSION_FAILED;
                if (dest) {
                    pfnFree(dest, pCallbackContext);
                    dest = NULL;
                }
                *destLen = 0;
            }

        }

    } 
    else 
    { // !entry
        DEBUG && ({
            sprintf(STR_LIM, "entry %s was not found!", name);
            puts(STR_LIM);
        });
    }


get_entry_error:

    //cleanup
    if (retval || !dest) 
    {
        DEBUG && ({
            sprintf(STR_LIM, "getEntryData failed!");
            puts(STR_LIM);
        });

        if (dest) {
            pfnFree(dest, pCallbackContext);
            dest = NULL;
        }
        if (destLen) {
            *destLen = 0;
        }
    }

    if (retval && destLen) {
        *destLen = 0;
    }

    if (src) {
        pfnFree(src, pCallbackContext);
    }

    hashmapUnlock(m_mpMap);

error:

    return dest;
}



/**
 * Creates root folder for unarchived content.
 *
 * @param *pf           - pointer to fullpath string
 * @param **p           - pointer to string where the PATH part will be stored
 * @param **f           - pointer to string where the FILE part will be stored
 * @param *delimiter    - is "/" on UNIX and "\\" on Windows
 * @param *outputdir    - pointer to the output directory
 *
 * ATTENTION: caller must free() **p and **f after usage !
 */
static int createRootFolder(char *fp, char **p, char **f, char *delimiter, char *outputdir) 
{
    int error = 0;

    // get the file-name
    splitFullpathIntoPathAndFile(fp, p, f, delimiter);

    // remove the file-extension part from the file-name
    strcpy(fp, *f);
    splitFullpathIntoPathAndFile(fp, p, f, ".");
    if (*p != NULL) {
        //
        // TODO: may be received from JavaScript side, so probably needs sanitation etc
        //

        // prepend the path with an output directory name
        strcat(outputdir, delimiter);
        strcat(outputdir, *p);

        // call the system's MKDIR
        char *syscmdargs[4] = {"mkdir", "-p", outputdir, NULL};
        error = makeSystemCall("mkdir", syscmdargs, outputdir);
    }

    return error;
}



/**
 * Writes an unzipped entry to the file.
 *
 * @param *pf           - pointer to fullpath string
 * @param **p           - pointer to string where the PATH part will be stored
 * @param **f           - pointer to string where the FILE part will be stored
 * @param *delimiter    - is "/" on UNIX and "\\" on Windows
 * @param *outputdir    - pointer to the output directory
 *
 * ATTENTION: caller must free() **p and **f after usage !
 */
static int writeUnzippedEntryToFile(char *fp, char **p, char **f, char *delimiter, char *outputdir) 
{
    char dirpath[MAX_FILENAME];
    unsigned char *data = NULL;
    unsigned long size  = 0;
    int error           = 0;

    splitFullpathIntoPathAndFile(fp, p, f, delimiter);
    
    if (*p != NULL) 
    {    
        strcpy(dirpath, outputdir);
        strcat(dirpath, delimiter);
        strcat(dirpath, *p);
        DEBUG && ({
            sprintf(STR_LIM, "\tDIR:  %s", dirpath);
            puts(STR_LIM);
        });

        // call the system's MKDIR
        char *syscmdargs[4] = {"mkdir", "-p", dirpath, NULL};
        error = makeSystemCall("mkdir", syscmdargs, dirpath);
        if (-1 == error) {
            DEBUG && ({
                sprintf(STR_LIM, ANSI_COLOR_RED"\n\tERROR in writeUnzippedEntryToFile(): system call to MKDIR failed!\n"ANSI_COLOR_RESET);
                puts(STR_LIM);
            });
            goto err;
        }

    } 
    else 
    {
        strcpy(dirpath, outputdir);
    
        DEBUG && ({
            sprintf(STR_LIM, "\tDIR:  %s", dirpath);
            puts(STR_LIM);
        });
    }
    
    DEBUG && ({
        sprintf(STR_LIM, "\tFILE: %s\n", *f);
        puts(STR_LIM);
    });

    if (*f != NULL) {
        // touch the file within the fresh backed directory
        data = getEntryData(fp, &size, 0, 0, 0);
        strcat(dirpath, delimiter);
        strcat(dirpath, *f);
        FILE* fp = fopen(dirpath, "w+");// read/write
        fwrite(data, size, 1, fp);
        fclose(fp);
    }

err:

    if(data) {
        free(data);
    }

    return error;
}





//--------------------------- MAIN SECTION -------------------------------

int main(int argc, char **argv)
{
    char path2archive[MAX_FILENAME];
    char outputdir[MAX_FILENAME];

    char *path          = NULL;
    char *file          = NULL;

    double start        = 0.0;
    double end          = 0.0;
    int error           = 0;


    if (argc < 3) {
        sprintf(STR_LIM, "usage:\n%s {path to file to unzip} {path to directory where to place the unarchived content}\n", argv[0]);
        puts(STR_LIM);
        return -1;
    }

    if (strlen(argv[1]) > MAX_FILENAME) {
        sprintf(STR_LIM, "ERROR: File path is TOO LONG: %lu\n", strlen(argv[1]));
        puts(STR_LIM);
        return -1;
    }

    if (strlen(argv[2]) > MAX_FILENAME) {
        sprintf(STR_LIM, "ERROR: File path is TOO LONG: %lu\n", strlen(argv[2]));
        puts(STR_LIM);
        return -1;
    }


    strcpy(path2archive, argv[1]);
    strcpy(outputdir,    argv[2]);

    //
    // TODO: тут нужны проверки корректности присланного пути к файлу, чтобы он имел расширение .DOCS etc
    //
/*
    // get the file extension
    int fplen = strlen(argv[1]);
    char name[fplen];
    strcpy(name, argv[1]);
    char extension[16];
    char* peek = name + name[strlen(name) - 1];
    while (peek >= name) {
        if (*peek == '.') {
            strcpy(extension, peek);
            break;
        }
        peek--;
    }
    sprintf(STR_LIM, "\n\tPath [%s]  =>  has the file extension [%s]\n", name, extension);
    puts(STR_LIM);
*/


    start = clock();

    // open the archive
    error = openZipArchive(path2archive);
    if (error) {
        sprintf(STR_LIM, ANSI_COLOR_RED"\n\tERROR CODE: %d\n"ANSI_COLOR_RESET, error);
        puts(STR_LIM);
        //
        // TODO: an exception message will be send to JavaScript side ?
        //
        goto xep;
    }


    // create the root folder for unarchived content
    error = createRootFolder(argv[1], &path, &file, DIR_DELIMITER, outputdir);
    if(-1 == error) {
        DEBUG && ({
            sprintf(STR_LIM, ANSI_COLOR_RED"\n\tERROR: createRootFolder() failed, returned code %d\n\n"ANSI_COLOR_RESET, error);
            puts(STR_LIM);
        });
        goto xep;
    }

    //
    // TODO: for large archives remember the initial m_nEntriesCount and then decrement it 
    // after every successfully MKDIR/fopen etc - to allow the "progress bar" on the JS side
    //
    // let archive is treated as "large" if it contains more than 500 entries:
    // if(m_nEntriesCount > 500) ... - выдавать юзеру ворнинг о кол-ве места, которое займёт на диске раззипованный контент
    // 
    sprintf(STR_LIM, "\n\tm_nEntriesCount = %d\n", m_nEntriesCount);
    puts(STR_LIM);
    //

    // iterate through entitites and unarchive them into the root folder
    if (m_arrEntries) {
        for (int i = 0; i < m_nEntriesCount; i++) {
            if (m_arrEntries[i].name) {
                error = writeUnzippedEntryToFile(m_arrEntries[i].name, &path, &file, DIR_DELIMITER, outputdir);
                if(-1 == error) {
                    DEBUG && ({
                        sprintf(STR_LIM, ANSI_COLOR_RED"\n\tERROR: createRootFolder() failed, returned code %d\n\n"ANSI_COLOR_RESET, error);
                        puts(STR_LIM);
                    });
                    goto xep;
                }
            }
        }
    }


    end = clock();

    sprintf(STR_LIM, ANSI_COLOR_GREEN"\n\tWELL DONE in %.6lf sec.\n\tfor unarchived content look into:\n\t%s\n\n"ANSI_COLOR_RESET, (end-start)/CLOCKS_PER_SEC, outputdir);
    puts(STR_LIM);

xep:

    if(path) free(path);
    if(file) free(file);

    cleanup();

    return 0;
}
