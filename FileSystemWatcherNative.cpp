//******************************************************************************
//**  FileSystemWatcherNative Class
//******************************************************************************
/**
 *   JNI code used to monitor changes made to a directory.
 *
 ******************************************************************************/

// This file needs to be compiled with _UNICODE flag, since java string uses unicode.

#include "stdafx.h"
#include "FileSystemWatcherNative.h"
#include <sstream>
#include <ctime>
using namespace std;


BOOL bWatchSubtree;      //flag used to set whether to watch subdirectories
DWORD dwNotifyFilter;    //notification filter
HANDLE hDirectory;       //handle to the directory being monitored
wstring sDirectory;      //a string representing the path to the directory

size_t nBufSize = 32*1024;
FILE_NOTIFY_INFORMATION* pBuffer = (FILE_NOTIFY_INFORMATION*)calloc(1, nBufSize);
FILE_NOTIFY_INFORMATION* pBufferCurrent;


//**************************************************************************
//** FindFirstChangeNotification
//**************************************************************************
/**
 * Class:     FileSystemWatcherNative
 * Method:    FindFirstChangeNotification
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jlong JNICALL Java_javaxt_io_FileSystemWatcherNative_FindFirstChangeNotification
(JNIEnv* env, jclass, jstring filename, jboolean javaWatchSubtree, jint javaNotifyFilter)
{

  //Convert jstring to wstring
    const jchar *_filename = env->GetStringChars(filename, 0);
    jsize len = env->GetStringLength(filename);
    sDirectory.assign(_filename, _filename + len);
    env->ReleaseStringChars(filename, _filename);

    

  //Update Inputs
    bWatchSubtree = (BOOL)javaWatchSubtree;
    dwNotifyFilter = //(DWORD)javaNotifyFilter;
        FILE_NOTIFY_CHANGE_LAST_WRITE|  // Triggered when a file or directory has been modified
        FILE_NOTIFY_CHANGE_DIR_NAME|    // Triggered when a directory has been created or deleted
        FILE_NOTIFY_CHANGE_FILE_NAME;   // Triggered when a file has been created or deleted


  //Call FindFirstChangeNotification
    HANDLE handle = FindFirstChangeNotificationW(sDirectory.c_str(), bWatchSubtree, dwNotifyFilter);
    if (handle == INVALID_HANDLE_VALUE || handle == (HANDLE)ERROR_INVALID_FUNCTION){
        DWORD errorCode = GetLastError();
        stringstream ss;
        ss << "FindFirstChangeNotification failed. Error Code: " << errorCode;
        const char* msg = (const char*)( ss.str().c_str());
        jclass exceptionClass = env->FindClass("java/lang/Exception");
        env->ThrowNew(exceptionClass, msg );
    }


  //Create File Handle
    hDirectory = CreateFileW(
        sDirectory.c_str(),                  // pointer to the directory
        GENERIC_READ,                        // access (read/write) mode
        FILE_SHARE_READ|
        FILE_SHARE_WRITE|
        FILE_SHARE_DELETE,                  // share mode
        NULL,                               // security descriptor
        OPEN_EXISTING,                      // how to create
        FILE_FLAG_BACKUP_SEMANTICS,         // file attributes
        NULL                                // file with attributes to copy
    );


  //Return FindFirstChangeNotification Handle
    return (jlong)handle;
}


//**************************************************************************
//** FindNextChangeNotification
//**************************************************************************
/**
 * Class:     FileSystemWatcherNative
 * Method:    FindNextChangeNotification
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_javaxt_io_FileSystemWatcherNative_FindNextChangeNotification
(JNIEnv *javaEnv, jclass, jlong handle)
{
    if (!FindNextChangeNotification((HANDLE) handle)){
        DWORD errorCode = GetLastError();
        stringstream ss;
        ss << "FindNextChangeNotification failed. Error Code: " << errorCode;
        const char* msg = (const char*)( ss.str().c_str());
        jclass exceptionClass = javaEnv->FindClass("java/lang/Exception");
        javaEnv->ThrowNew(exceptionClass, msg);
    }
}


//**************************************************************************
//** FindCloseChangeNotification
//**************************************************************************
/**
 * Class:     FileSystemWatcherNative
 * Method:    FindCloseChangeNotification
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_javaxt_io_FileSystemWatcherNative_FindCloseChangeNotification
(JNIEnv *javaEnv, jclass, jlong handle)
{

    if (!FindCloseChangeNotification((HANDLE) handle)){
        DWORD errorCode = GetLastError();
        stringstream ss;
        ss << "FindCloseChangeNotification failed. Error Code: " << errorCode;
        const char* msg = (const char*)( ss.str().c_str());
        jclass exceptionClass = javaEnv->FindClass("java/lang/Exception");
        javaEnv->ThrowNew(exceptionClass, msg);
    }


  //Close File Handle
    if (hDirectory!=NULL){
        CloseHandle(hDirectory);
    }
}


//**************************************************************************
//** WaitForSingleObject
//**************************************************************************
/**
 * Class:     FileSystemWatcherNative
 * Method:    WaitForSingleObject
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_javaxt_io_FileSystemWatcherNative_WaitForSingleObject
(JNIEnv *javaEnv , jclass, jlong hWaitHandle, jint waitTimeoutMillis)
{
  return WaitForSingleObject((HANDLE) hWaitHandle, (DWORD) waitTimeoutMillis);
}


std::wstring s2ws(const std::string& s){
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}


//**************************************************************************
//** ReadDirectoryChangesW
//**************************************************************************
/**
 * Class:     FileSystemWatcherNative
 * Method:    ReadDirectoryChangesW
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_FileSystemWatcherNative_ReadDirectoryChangesW
(JNIEnv *env, jclass)
{

    wstringstream ss;
    DWORD BytesReturned;

    if (ReadDirectoryChangesW(
        hDirectory,                     // handle to directory
        pBuffer,                        // read results buffer
        nBufSize,                       // length of buffer
        bWatchSubtree,                  // WatchSubtree
        dwNotifyFilter,                 // notification filter
        &BytesReturned,                 // bytes returned
        NULL,                           // overlapped buffer
        NULL                            // completion routine
    )){

      //Iterate through the file changes
        pBufferCurrent = pBuffer;
        while(pBufferCurrent){

          //Get current timestamp
            time_t d = time(NULL);
            string date( ctime(&d) );
            date.erase(date.find_last_not_of(" \t\n")+1); //right trim


          //Get action
            wstring action = L"";
            switch(pBufferCurrent->Action){
                case FILE_ACTION_ADDED:    action = L"Create"; break;
                case FILE_ACTION_REMOVED:  action = L"Delete"; break;
                case FILE_ACTION_MODIFIED: action = L"Modify"; break;
                case FILE_ACTION_RENAMED_OLD_NAME: action = L"Rename"; break;
                case FILE_ACTION_RENAMED_NEW_NAME: action = L"Renam2"; break;
            }


          //Get filename
            WCHAR* _filename = pBufferCurrent->FileName;
            int len = (int)pBufferCurrent->FileNameLength/2;
            wstring filename;
            filename.assign(_filename, _filename + len);


          //Join date, action, and filename into 1 event string        
            ss << L"[" << s2ws(date) << L"] " << action << L" " << sDirectory << filename << L"\n";


          //Update pBufferCurrent
            if (pBufferCurrent->NextEntryOffset)
                pBufferCurrent = (FILE_NOTIFY_INFORMATION*)(((BYTE*)pBufferCurrent) + pBufferCurrent->NextEntryOffset);
            else
                pBufferCurrent = NULL;


        } //end while pBufferCurrent
    } 

    wstring event = ss.str();
    int len = event.size();
    jchar* raw = new jchar[len];
    memcpy(raw, event.c_str(), len*sizeof(wchar_t));
    jstring result = env->NewString(raw, len);
    delete[] raw;
    return result;           
}