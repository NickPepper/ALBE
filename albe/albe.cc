// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Agile Fusion. All rights reserved.

/// @file albe.cc
/// @author Nick Pershin

#include <sstream>
#include <string>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/ppb_file_io.h"
#include "ppapi/cpp/file_io.h"
#include "ppapi/cpp/file_ref.h"
#include "ppapi/cpp/file_system.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/cpp/directory_entry.h"
#include "ppapi/cpp/message_loop.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"
#include <ppapi/cpp/completion_callback.h>
#include "nacl_io/nacl_io.h"

// shared libs
#include "dumb.h"
#include "unarch.h"

#include "albe.h"


/// The AlbeInstance class.
/// One of these exists for each instance of our NaCl module on the web page.
/// The browser will ask the Module object to create a new Instance
/// for each occurrence of the <embed> tag that has these attributes:
///     type="application/x-nacl"
///     src="albe.nmf"
class AlbeInstance : public pp::Instance 
{
public:

    explicit AlbeInstance(PP_Instance instance) : pp::Instance(instance),
                                                callback_factory_(this),
                                                localpersistent_fs_(this, PP_FILESYSTEMTYPE_LOCALPERSISTENT),
                                                localtemp_fs_(this, PP_FILESYSTEMTYPE_LOCALTEMPORARY),
                                                localpersistent_fs_ready_(false),
                                                localtemp_fs_ready_(false),
                                                // shared libs
                                                //c_utils_so_(NULL),
                                                unarch_so_(NULL),
                                                dumb_so_(NULL),
                                                unarch_(NULL),
                                                dumb_(NULL),
                                                // thread for shared libraries
                                                so_thread_(this),
                                                // file thread
                                                file_thread_(this) {}

    virtual ~AlbeInstance() {
        dlclose(dumb_so_);
        dlclose(unarch_so_);
        //dlclose(c_utils_so_);
        so_thread_.Join();
        file_thread_.Join();
    }


    virtual bool Init(uint32_t/*argc*/, const char* []/*argn*/, const char* []/*argv*/) 
    {
        // start the thread for file I/O operations
        file_thread_.Start();

        // since this is the first operation we perform there,
        // and because we do everything on the file_thread_ synchronously,
        // below ensures that the FileSystems are open before any FileIO operations execute
        
        // open the local persistent file system on the file_thread_
        file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::OpenLocalPersistentFileSystem));
        // open the local temporary file system on the file_thread_
        file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::OpenLocalTemporaryFileSystem));
        

        // start the thread to communicate with shared libraries 
        so_thread_.Start();

        nacl_io_init_ppapi(pp_instance(), pp::Module::Get()->get_browser_interface());
        
        // Mount a HTTP mount at /http.
        // All reads from /http/* will read from the server.
        mount("", "/http", "httpfs", 0, "");

        ThrowStatusMessage(std::string("AlbeInstance::Init(): Spawning thread to cache .so files..."));

        so_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::LoadLibrary));
        //so_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::LoadLibrary, this));
        
        return true;
    }


private:

    pp::CompletionCallbackFactory<AlbeInstance> callback_factory_;
    pp::FileSystem localpersistent_fs_;
    pp::FileSystem localtemp_fs_;
    
    // Indicates whether different filesystems were opened successfully.
    // We only read/write this on the file_thread_.
    bool localpersistent_fs_ready_;
    bool localtemp_fs_ready_;

    // pointers to shared libraries
    //void* c_utils_so_;
    void* unarch_so_;
    void* dumb_so_;
    TYPE_unarch unarch_;
    TYPE_dumb dumb_;

    // do all shared libraries operations on a worker thread
    pp::SimpleThread so_thread_;

    // do all file operations on an other worker thread
    pp::SimpleThread file_thread_;



    /// This method is called on a worker thread, and will call dlopen() to load the shared object
    void LoadLibrary(int32_t) 
    //void LoadLibrary(int32_t, AlbeInstance* inst)
    {
        std::string message;

        // A shared library called libunarch.so gets included in the .nmf file
        // and is therefore directly loadable with dlopen()
        unarch_so_ = dlopen("libunarch.so", RTLD_LAZY);
        if (unarch_so_ != NULL) 
        {
            intptr_t offset = (intptr_t) dlsym(unarch_so_, "SaySomething");
            unarch_ = (TYPE_unarch) offset;
            if (NULL == unarch_) {
                message = "dlsym() returned NULL: ";
                message += dlerror();
                ThrowErrorMessage(message, -777);
                return;
            }
            ThrowStatusMessage(std::string("Loaded OK: libunarch.so"));
        } 
        else {
            ThrowErrorMessage(std::string("libunarch.so did not load"), -777);
        }
/*
        // libalbe_c_utils.so gets included in the .nmf file
        // and is therefore directly loadable with dlopen()
        c_utils_so_ = dlopen("libalbe_c_utils.so", RTLD_LAZY);
        if (c_utils_so_ != NULL) {
            ThrowStatusMessage(std::string("Loaded OK: libalbe_c_utils.so"));
        } else {
            ThrowErrorMessage(std::string("libalbe_c_utils.so did not load"), -777);
        }
*/
        // A shared library called libdumb.so is NOT included in the .nmf file 
        // and is loaded via an http mount using the nacl_io library
        const char dumb_so_path[] = "/http/glibc/" CONFIG_NAME "/libdumb_" NACL_ARCH ".so";
        dumb_so_ = dlopen(dumb_so_path, RTLD_LAZY);
        if (dumb_so_ != NULL) 
        {
            intptr_t offset = (intptr_t) dlsym(dumb_so_, "SaySomething");
            dumb_ = (TYPE_dumb) offset;
            if (NULL == dumb_) {
                message = "dlsym() returned NULL: ";
                message += dlerror();
                ThrowErrorMessage(message, -777);
                return;
            }
            ThrowStatusMessage(std::string("Loaded OK: libdumb.so"));
        } 
        else {
            ThrowErrorMessage(std::string("libdumb.so did not load"), -777);
        }

    }



    /// Handler for messages coming in from the browser via postMessage().
    /// The @a var_message can contain anything: a JSON string; a string that encodes method names and arguments; etc.
    /// (currently we use JSON)
    ///
    /// Here we use messages to communicate with the user interface
    ///
    /// @param[in] var_message The message posted by the browser.
    virtual void HandleMessage(const pp::Var& var_message)
    {
        // accept only JS-object messages
        if(!var_message.is_dictionary()) {
            ThrowErrorMessage(std::string("in HandleMessage() -> received message is not a valid JavaScript Object!\n\tmessage: ["+var_message.AsString()+"]"), -777);
            return;
        }

        pp::VarDictionary var_dict(var_message);
        std::string cmd = var_dict.Get("cmd").AsString();
        if(cmd.empty()) {
            ThrowErrorMessage(std::string("in HandleMessage() -> empty 'cmd' parameter!"), -777);
            return;
        }

        // dispatch the 'cmd'
        if(cmd == kSaveToExterntalPrefix) 
        {
            pp::Resource filesystem_resource = var_dict.Get("filesystem").AsResource();
            pp::FileSystem filesystem(filesystem_resource);
            std::string full_path = var_dict.Get("fullPath").AsString();
            std::string save_path = full_path + "/Привет от Native Client.txt";
            std::string contents = "Жёсткий диск, тебе мягкий привет от НаКла! :)\n";
            
            file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::WriteFileToExternalFS, filesystem, save_path, contents));
        }
        else if(cmd == kLoadExternalPrefix)
        {
            pp::Resource filesystem_resource = var_dict.Get("filesystem").AsResource();
            pp::FileSystem filesystem(filesystem_resource);
            std::string full_path = var_dict.Get("fullPath").AsString();
            
            file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::OpenFileFromExternalFS, filesystem, full_path));
        }
        else if(cmd == kLoadPrefix)
        {
            std::string file_name = var_dict.Get("fileName").AsString();
            file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::LoadFileFromLocalFS, file_name));
        }
        else if(cmd == kSavePrefix)
        {
            std::string file_name       = var_dict.Get("fileName").AsString();
            std::string file_contents   = var_dict.Get("contents").AsString();
            file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::SaveFileToLocalFS, file_name, file_contents));
        }
        else if(cmd == kDeletePrefix)
        {
            std::string file_name       = var_dict.Get("fileName").AsString();
            file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::DeleteFileFromLocalFS, file_name));
        }
        else if(cmd == kListPrefix)
        {
            const std::string& dir_name = var_dict.Get("dirName").AsString();
            file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::ListDirectoryOnLocalFS, dir_name));
        }
        else if(cmd == kMakeDirPrefix)
        {
            const std::string& dir_name = var_dict.Get("dirName").AsString();
            file_thread_.message_loop().PostWork(callback_factory_.NewCallback(&AlbeInstance::MakeDirOnLocalFS, dir_name));
        }
        else if(cmd == kSharedLibDlopen) 
        {
            if (NULL == unarch_) {
                ThrowErrorMessage("Unarch library not loaded", -777);
                return;
            }
            std::string ssmsg = "Unarch SaySomething() says: ";
            ssmsg += unarch_();
            ssmsg += "!";
            ThrowStatusMessage(ssmsg);
        }
        else if(cmd == kSharedLibHttpMount)
        {
            if (NULL == dumb_) {
                ThrowErrorMessage("Dumb library not loaded", -777);
                return;
            }
            std::string ssmsg = "Dumb SaySomething() says: ";
            ssmsg += dumb_();
            ssmsg += "!";
            ThrowStatusMessage(ssmsg);
            /*
            std::string s = "Reverse me!";
            char* result = unarch_(s.c_str());
            std::string unmsg = "\tString to reverse: \""+s+"\"\n\tReversed string: \"";
            unmsg += result;
            unmsg += "\"";
            free(result);
            ThrowStatusMessage(unmsg);
            */
        }
        else
        {
            ThrowErrorMessage(std::string("Unknown cmd parameter from JavaScript is received: ["+cmd+"]"), -777);
        }
    }



    /// Opens a given file from the external file system (i.e. from the real user's physical disk)
    /// @param[int32_t] result (for callback)
    /// @param[const pp::FileSystem&] filesystem
    /// @param[const std::string&] path
    void OpenFileFromExternalFS(int32_t, 
                    const pp::FileSystem& filesystem,
                    const std::string& path) 
    {
        pp::FileRef ref(filesystem, path.c_str());
        pp::FileIO file(this);

        int32_t open_result = file.Open(ref, PP_FILEOPENFLAG_READ, pp::BlockUntilComplete());

        if (open_result != PP_OK) {
            IOErrorHandler(open_result, path);
            return;
        }

        // TODO: A REAL OPENING (!)
        ThrowStatusMessage(std::string("NaCl OK: Successfully opened file "+path));

        file.Close();
    }



    /// Writes a given file to the external file system (i.e. to the real user's physical disk)
    /// @param[int32_t] result (for callback)
    /// @param[const pp::FileSystem&] filesystem
    /// @param[const std::string&] path
    /// @param[const std::string&] contents
    void WriteFileToExternalFS(int32_t, 
                    const pp::FileSystem& filesystem,
                    const std::string& path,
                    const std::string& contents) 
    {
        pp::FileRef ref(filesystem, path.c_str());
        pp::FileIO file(this);

        int32_t open_result = file.Open(ref, PP_FILEOPENFLAG_CREATE | PP_FILEOPENFLAG_EXCLUSIVE | PP_FILEOPENFLAG_WRITE | PP_FILEOPENFLAG_TRUNCATE, pp::BlockUntilComplete());

        if (open_result != PP_OK) {
            IOErrorHandler(open_result, path);
            return;
        }

        int64_t offset          = 0;
        int32_t bytes_written   = 0;
        do {
            bytes_written = file.Write(offset, contents.data() + offset, contents.length(), pp::BlockUntilComplete());
            if (bytes_written > 0) {
                offset += bytes_written;
            } else {
                ThrowErrorMessage(std::string("Failed to write file "+path), -777);
                return;
            }
        } while (bytes_written < static_cast<int64_t>(contents.length()));

        // all bytes have been written, flush the write buffer to complete
        int32_t flush_result = file.Flush(pp::BlockUntilComplete());
        if (flush_result != PP_OK) {
            ThrowErrorMessage(std::string("Failed to flush file "+path), -777);
            return;
        }
        ThrowStatusMessage(std::string("Successfully wrote file "+path));
    }



    /// IOErrorHandler
    /// @param[const int32_t] result
    /// @param[const std::string&] path
    virtual void IOErrorHandler(const int32_t result, const std::string& path) 
    {
        switch(result) 
        {
            // see https://developer.chrome.com/native-client/pepper_dev/c/group___enums
            // for all error codes
            // 
            // and http://src.chromium.org/chrome/trunk/src/ppapi/c/pp_errors.h
            // for explanations
            //
            case PP_ERROR_NOACCESS:
            {// errcode -7
                ThrowErrorMessage(std::string("No access for "+path+", probably you have no rights to read/write in this directory."), result);
            }
            break;

            case PP_ERROR_NOMEMORY:
            {// errcode -8
                ThrowErrorMessage(std::string("Not enough memory while opening "+path), result);
            }
            break;

            case PP_ERROR_NOSPACE:
            {// errcode -9
                ThrowErrorMessage(std::string("Insufficient storage space while opening "+path), result);
            }
            break;

            case PP_ERROR_NOQUOTA:
            {// errcode -10
                ThrowErrorMessage(std::string("Insufficient storage quota while opening "+path), result);
            }
            break;

            case PP_ERROR_FILENOTFOUND:
            {// errcode -20
                ThrowErrorMessage(std::string("File "+path+" not found!"), result);
            }
            break;

            case PP_ERROR_FILEEXISTS:
            {// errcode -21
                ThrowErrorMessage(std::string("File "+path+" already exists!"), result);
            }
            break;

            case PP_ERROR_FILETOOBIG:
            {// errcode -22
                ThrowErrorMessage(std::string("File "+path+" is too big!"), result);
            }
            break;

            case PP_ERROR_FILECHANGED:
            {// errcode -23
                ThrowErrorMessage(std::string("File "+path+" has been modified unexpectedly!"), result);
            }
            break;

            case PP_ERROR_NOTAFILE:
            {// errcode -24
                ThrowErrorMessage(std::string("The pathname "+path+" does not reference a file!"), result);
            }
            break;

            default:
            {
                // Errors are negative valued.
                // Callers should treat all negative values as a failure, even if it's not in the list,
                // since the possible errors in Pepper API are likely to expand and change over time.
                ThrowErrorMessage(std::string("Failed to open file "+path+" (see https://developer.chrome.com/native-client/pepper_dev/c/group___enums for all error codes)"), result);
            }
            break;
        }
    }


    /// Encapsulates the javascript communication protocol
    /// @param[const std::string&] message
    /// @param[int32_t] result
    void ThrowErrorMessage(const std::string& message, int32_t result) 
    {
        std::stringstream ss;
        ss << "ERR|" << message << " -- Error #: " << result;
        PostMessage(ss.str());
    }


    /// Encapsulates the javascript communication protocol
    /// @param[const std::string&] message
    void ThrowStatusMessage(const std::string& message) 
    {
        std::stringstream ss;
        ss << "STAT|" << message;
        PostMessage(ss.str());
    }


    void OpenLocalPersistentFileSystem(int32_t)
    {
        int32_t rv = localpersistent_fs_.Open(1024 * 1024, pp::BlockUntilComplete());
        if(rv == PP_OK) {
            localpersistent_fs_ready_ = true;
            ThrowStatusMessage("Local persistent filesystem is ready.");
        } else {
            ThrowErrorMessage("Failed to open the local file system", rv);
        }
    }


    void OpenLocalTemporaryFileSystem(int32_t)
    {
        int32_t rv = localtemp_fs_.Open(1024 * 1024, pp::BlockUntilComplete());
        if(rv == PP_OK) {
            localtemp_fs_ready_ = true;
            ThrowStatusMessage("Local temporary filesystem is ready.");
        } else {
            ThrowErrorMessage("Failed to open the temporary file system", rv);
        }
    }


    void LoadFileFromLocalFS(int32_t, const std::string& file_name)
    {
        if(!localpersistent_fs_ready_) {
            ThrowErrorMessage("File system is not open", PP_ERROR_FAILED);
            return;
        }

        pp::FileRef ref(localpersistent_fs_, file_name.c_str());
        pp::FileIO file(this);

        int32_t open_result = file.Open(ref, PP_FILEOPENFLAG_READ, pp::BlockUntilComplete());
        if(open_result == PP_ERROR_FILENOTFOUND) {
            ThrowErrorMessage("File not found", open_result);
            return;
        } else if(open_result != PP_OK) {
            ThrowErrorMessage("Open file for read failed", open_result);
            return;
        }

        PP_FileInfo info;
        int32_t query_result = file.Query(&info, pp::BlockUntilComplete());
        if(query_result != PP_OK) {
            ThrowErrorMessage("File query failed", query_result);
            return;
        }

        // FileIO.Read() can only handle int32 sizes
        if(info.size > INT32_MAX) {
            ThrowErrorMessage("File too big", PP_ERROR_FILETOOBIG);
            return;
        }

        std::vector<char> data(info.size);
        int64_t offset          = 0;
        int32_t bytes_read      = 0;
        int32_t bytes_to_read   = info.size;
        
        while(bytes_to_read > 0) {
            bytes_read = file.Read(offset, &data[offset], data.size() - offset, pp::BlockUntilComplete());
            if(bytes_read > 0) {
                offset += bytes_read;
                bytes_to_read -= bytes_read;
            } else if(bytes_read < 0) {
                // if bytes_read < PP_OK then it indicates the error code
                ThrowErrorMessage("Read file failed", bytes_read);
                return;
            }
        }

        // done reading, send content to the user interface
        std::string string_data(data.begin(), data.end());
        //PostMessage("DISP|" + string_data);
        ThrowStatusMessage("Load success");
    }



    void SaveFileToLocalFS(int32_t, const std::string& file_name, const std::string& file_contents)
    {
        if(!localpersistent_fs_ready_) {
            ThrowErrorMessage("File system is not open", PP_ERROR_FAILED);
            return;
        }

        pp::FileRef ref(localpersistent_fs_, file_name.c_str());
        pp::FileIO file(this);

        int32_t open_result = file.Open(ref, PP_FILEOPENFLAG_WRITE | PP_FILEOPENFLAG_CREATE | PP_FILEOPENFLAG_TRUNCATE, pp::BlockUntilComplete());
        if(open_result != PP_OK) {
            ThrowErrorMessage("Open file for write failed", open_result);
            return;
        }

        // We have truncated the file to 0 bytes.
        // So we need only write if file_contents is non-empty.
        if(!file_contents.empty()) 
        {
            if(file_contents.length() > INT32_MAX) {
                ThrowErrorMessage("File too big", PP_ERROR_FILETOOBIG);
                return;
            }

            int64_t offset          = 0;
            int32_t bytes_written   = 0;
            do {
                bytes_written = file.Write(offset, file_contents.data() + offset, file_contents.length(), pp::BlockUntilComplete());
                if(bytes_written > 0) {
                    offset += bytes_written;
                } else {
                    ThrowErrorMessage("Write file failed", bytes_written);
                    return;
                }
            } while (bytes_written < static_cast<int64_t>(file_contents.length()));
        }

        // all bytes have been written, flush the write buffer to complete
        int32_t flush_result = file.Flush(pp::BlockUntilComplete());
        if(flush_result != PP_OK) {
            ThrowErrorMessage("Fail to flush", flush_result);
            return;
        }
        ThrowStatusMessage("Save success");
    }



    void DeleteFileFromLocalFS(int32_t, const std::string& file_name)
    {
        if(!localpersistent_fs_ready_) {
            ThrowErrorMessage("File system is not open", PP_ERROR_FAILED);
            return;
        }

        pp::FileRef ref(localpersistent_fs_, file_name.c_str());

        int32_t result = ref.Delete(pp::BlockUntilComplete());
        if(result == PP_ERROR_FILENOTFOUND) 
        {
            ThrowStatusMessage("File/Directory not found");
            return;
        } 
        else if(result != PP_OK) 
        {
            ThrowErrorMessage("Deletion failed", result);
            return;
        }

        ThrowStatusMessage("Delete success");
    }



    void ListDirectoryOnLocalFS(int32_t, const std::string& dir_name)
    {
        if(!localpersistent_fs_ready_) {
            ThrowErrorMessage("File system is not open", PP_ERROR_FAILED);
            return;
        }

        pp::FileRef ref(localpersistent_fs_, dir_name.c_str());

        // pass ref along to keep it alive
        ref.ReadDirectoryEntries(callback_factory_.NewCallbackWithOutput(&AlbeInstance::ListDirectoryCallback, ref));
    }



    // pp::FileRef is unused_ref
    void ListDirectoryCallback(int32_t result, const std::vector<pp::DirectoryEntry>& entries, pp::FileRef)
    {
        if(result != PP_OK) {
            ThrowErrorMessage("List failed", result);
            return;
        }

        std::stringstream ss;
        ss << "LIST";
        for(size_t i = 0; i < entries.size(); ++i) {
            pp::Var name = entries[i].file_ref().GetName();
            if(name.is_string()) {
                ss << "|" << name.AsString();
            }
        }
        PostMessage(ss.str());
        ThrowStatusMessage("List success");
    }



    void MakeDirOnLocalFS(int32_t, const std::string& dir_name)
    {
        if(!localpersistent_fs_ready_) {
            ThrowErrorMessage("File system is not open", PP_ERROR_FAILED);
            return;
        }
        pp::FileRef ref(localpersistent_fs_, dir_name.c_str());

        int32_t result = ref.MakeDirectory(PP_MAKEDIRECTORYFLAG_NONE, pp::BlockUntilComplete());
        if(result != PP_OK) {
            ThrowErrorMessage("Make directory failed", result);
            return;
        }
        ThrowStatusMessage("Make directory success");
    }



}; // class Instance



/// The Module class.
/// The browser calls the CreateInstance() method to create an instance of our NaCl module on the web page.
/// The browser creates a new instance for each <embed> tag with type="application/x-nacl".
class AlbeModule : public pp::Module 
{
public:

    AlbeModule() : pp::Module() {}
    virtual ~AlbeModule() {}

    /// Create and return an AlbeInstance object.
    /// @param[in] instance The browser-side instance.
    /// @return the plugin-side instance.
    virtual pp::Instance* CreateInstance(PP_Instance instance) {
        return new AlbeInstance(instance);
    }
}; // class AlbeModule


/// Factory function called by the browser when the module is first loaded.
/// The browser keeps a singleton of this module.
/// It calls the CreateInstance() method on the object you return to make instances.
/// There is one instance per <embed> tag on the page.
/// This is the main binding point for our NaCl module with the browser.
namespace pp {
    Module* CreateModule() { 
        return new AlbeModule(); 
    }
}
