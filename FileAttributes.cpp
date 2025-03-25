#include "stdafx.h"
#include "FileAttributes.h"
#include <sstream>
#include <lm.h>
//#include <iostream> //for cout
using namespace std;

//******************************************************************************
//**  FileAttributes Class
//******************************************************************************
/**
 *   JNI code used to list files, network drives, and file attributes on Windows
 *   file systems. Requires "mpr.lib" and "Netapi32.lib" for network IO.
 *
 ******************************************************************************/


std::istringstream &operator >>(std::istringstream &iss, __int64 &n){
    sscanf(iss.str().c_str(), "%I64d", &n);
    return iss;
}

//**************************************************************************
//** from_string
//**************************************************************************
/** Allows users to append/concat an __int64 to a stringstream. This method
 *  is required for Visual Studio 6 compilers. Visual Studio 2003 and higher
 *  don't need this. GCC doesn't seem to need it either. Credit:
 *  http://www.codeguru.com/forum/archive/index.php/t-342716.html
 */
template <typename T> bool from_string(T &t, const std::string &s, std::ios_base & (*f)(std::ios_base&)){
    std::istringstream iss(s);
    iss >> f, iss >> t;
    return !iss.fail();
}


//**************************************************************************
//** date2int
//**************************************************************************
/** Converts FILETIME to a long value representing a date.
 */
__int64 date2int(const FILETIME &ft){

    SYSTEMTIME stUTC, stLocal;
    
    // Convert the last-write time to local time.
    FileTimeToSystemTime(&ft, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    stringstream ss;
    ss << stLocal.wYear;
    if (stLocal.wMonth<10) ss << "0" << stLocal.wMonth;
    else ss << stLocal.wMonth;
    if (stLocal.wDay<10) ss << "0" << stLocal.wDay;
    else ss << stLocal.wDay;
    if (stLocal.wHour<10) ss << "0" << stLocal.wHour;
    else ss << stLocal.wHour;
    if (stLocal.wMinute<10) ss << "0" << stLocal.wMinute;
    else ss << stLocal.wMinute;
    if (stLocal.wSecond<10) ss << "0" << stLocal.wSecond;
    else ss << stLocal.wSecond;

    if (stLocal.wMilliseconds<100 && stLocal.wMilliseconds>10) ss << "0" << stLocal.wMilliseconds;
    else if (stLocal.wMilliseconds<10) ss << "00" << stLocal.wMilliseconds;
    else ss << stLocal.wMilliseconds;

    //cout << ss.str();
    //cout << "\n";
    __int64 val;
    bool bconvert = from_string<__int64>(val, ss.str(), std::dec); //ss >> val;
    return val;
}


//**************************************************************************
//** GetFileAttributesEx
//**************************************************************************
/** Returns an array of long values representing WIN32_FILE_ATTRIBUTE_DATA.
 *  Credit: http://www.cplusplus.com/forum/windows/24603/
 *
 * Class:     javaxt_io_File
 * Method:    GetFileAttributesEx
 * Signature: (Ljava/lang/String;)[J
 */
JNIEXPORT jlongArray JNICALL Java_javaxt_io_File_GetFileAttributesEx(JNIEnv *env, jclass, jstring filename)
{   

  //Convert jstring to wstring
    const jchar *_filename = env->GetStringChars(filename, 0);
    jsize len = env->GetStringLength(filename);
    wstring path;
    path.assign(_filename, _filename + len);
    env->ReleaseStringChars(filename, _filename);


  //Get attributes
    WIN32_FILE_ATTRIBUTE_DATA fileAttrs;
    BOOL result = GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileAttrs);
    if (!result) {
        jclass exceptionClass = env->FindClass("java/lang/Exception");
        env->ThrowNew(exceptionClass, "Exception Occurred");
    }


    /*
    typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
      DWORD    dwFileAttributes;
      FILETIME ftCreationTime;
      FILETIME ftLastAccessTime;
      FILETIME ftLastWriteTime;
      DWORD    nFileSizeHigh;
      DWORD    nFileSizeLow;
    } WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;
    */


  //Create an array to store the WIN32_FILE_ATTRIBUTE_DATA
    jlong buffer[6];
    buffer[0] = fileAttrs.dwFileAttributes;
    buffer[1] = date2int(fileAttrs.ftCreationTime);
    buffer[2] = date2int(fileAttrs.ftLastAccessTime);
    buffer[3] = date2int(fileAttrs.ftLastWriteTime);
    buffer[4] = fileAttrs.nFileSizeHigh;
    buffer[5] = fileAttrs.nFileSizeLow;

    jlongArray jLongArray = env->NewLongArray(6);
    env->SetLongArrayRegion(jLongArray, 0, 6, buffer);
    return jLongArray;
}



//**************************************************************************
//** GetTarget
//**************************************************************************
/** Returns the target of a WIndows Junction. Credit:
 *  http://www.flexhex.com/docs/articles/hard-links.phtml
 *
 * Class:     javaxt_io_File
 * Method:    GetTarget
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_File_GetTarget(JNIEnv *env, jclass, jstring filename)
{ 

  //Convert jstring to wstring
    const jchar *_filename = env->GetStringChars(filename, 0);
    jsize len = env->GetStringLength(filename);
    wstring path;
    path.assign(_filename, _filename + len);
    env->ReleaseStringChars(filename, _filename);


    LPCWSTR pPath = GetTarget(path.c_str());
    if (pPath==NULL) return NULL;
    else{
        jchar * jc = (jchar *) pPath;
        int i = wcslen(pPath);
        return env->NewString(jc, i);
    }
    
}



//**************************************************************************
//** GetSharedDrives - Requires Netapi32.lib
//**************************************************************************
/** Returns a list of shared drives found on a server
 *
 * Class:     javaxt_io_File
 * Method:    GetSharedDrives
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_File_GetSharedDrives(JNIEnv *env, jclass, jstring servername)
{


  //Convert jstring to wstring
    const jchar *_servername = env->GetStringChars(servername, 0);
    jsize x = env->GetStringLength(servername);
    wstring str;
    str.assign(_servername, _servername + x);
    env->ReleaseStringChars(servername, _servername);

    wstringstream ss;

    PSHARE_INFO_502 BufPtr,p;
    NET_API_STATUS res;
    DWORD er=0,tr=0,resume=0, i;
    do{
        res = NetShareEnum ((LPWSTR) str.c_str(), 502, (LPBYTE *) &BufPtr, MAX_PREFERRED_LENGTH, &er, &tr, &resume);
        if (res == ERROR_SUCCESS || res == ERROR_MORE_DATA){
            p=BufPtr;

          //Loop through the entries
            for (i=1; i<=er; i++) {

              //Iterate through the file changes
                wstring type = L"Unknown";
                bool ret = true;
                switch(p->shi502_type){
                    case STYPE_DISKTREE:  type = L"Disk"; break;
                    case STYPE_PRINTQ:    type = L"Printer"; ret = false; break;
                    case STYPE_DEVICE:    type = L"Device"; ret = false; break;
                    case STYPE_IPC:       type = L"IPC"; ret = false; break;
                    //The following 2 lines won't compile on my Windows XP x32
                    //case STYPE_SPECIAL:   type = L"Special"; break;
                    //case STYPE_TEMPORARY: type = L"Temp"; break;
                }

                wstring path = wstring(p->shi502_path);
                if (path.empty()) ret = false;

                if (ret) ss << wstring(p->shi502_netname) << L"\t" << type << L"\t" << path << L"\n";


                //if (IsValidSecurityDescriptor(p->shi502_security_descriptor))
                //   printf("Yes\n");
                //else
                //   printf("No\n");
                p++;
            }

        //Free the allocated buffer
          NetApiBufferFree(BufPtr);
        }
        else {
            stringstream ss;
            ss << res;
            jclass exceptionClass = env->FindClass("java/lang/Exception");
            env->ThrowNew(exceptionClass, ss.str().c_str());
        }
    }
    while (res==ERROR_MORE_DATA);
   

    wstring cstr = ss.str();
    int len = cstr.size();
    jchar* raw = new jchar[len];
    memcpy(raw, cstr.c_str(), len*sizeof(wchar_t));
    jstring result = env->NewString(raw, len);
    delete[] raw;
    return result;
}


//**************************************************************************
//** GetNetworkDrives
//**************************************************************************
/** Returns a list of network drives mounted on the host computer. This is
 *  similar to the "net use" command.
 *
 * Class:     javaxt_io_File
 * Method:    GetNetworkDrives
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_File_GetNetworkDrives(JNIEnv *env, jclass)
{
    wstringstream ss;
    HANDLE h;
    if (WNetOpenEnum(RESOURCE_REMEMBERED, RESOURCETYPE_DISK, RESOURCEUSAGE_ALL,  NULL, &h) == NO_ERROR) {
        DWORD is = 2;
        NETRESOURCE buf[26] = { 0 };
        DWORD c   = -1;
        DWORD s   = sizeof(buf);
        DWORD r;
        if ((r = WNetEnumResource(h, &c, buf, &s)) != NO_ERROR){
            jclass exceptionClass = env->FindClass("java/lang/Exception");
            env->ThrowNew(exceptionClass, "Exception Occurred");
        }
        for (int i = 0; i < c; ++i){
            NETRESOURCE ns = buf[i];
            ss << ns.lpLocalName << L"\t" << ns.lpRemoteName << L"\n";
        }
    }

    wstring cstr = ss.str();
    int len = cstr.size();
    jchar* raw = new jchar[len];
    memcpy(raw, cstr.c_str(), len*sizeof(wchar_t));
    jstring result = env->NewString(raw, len);
    delete[] raw;
    return result;
}


//**************************************************************************
//** GetFiles
//**************************************************************************
/** Returns a list of files found in the current directory. Note that the
 *  filename must end with "\*"
 *
 * Class:     javaxt_io_File
 * Method:    GetFiles
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_File_GetFiles(JNIEnv *env, jclass, jstring directory)
{
    HANDLE hFind;
    try {
    
      //Convert jstring to wstring
        const jchar *_directory = env->GetStringChars(directory, 0);
        jsize x = env->GetStringLength(directory);
        wstring path;  //L"C:\\temp\\*";  //Prepend "\\?\"
        path.assign(_directory, _directory + x);
        env->ReleaseStringChars(directory, _directory);

        if (x<2){
            jclass exceptionClass = env->FindClass("java/lang/Exception");
            env->ThrowNew(exceptionClass, "Invalid path, less than 2 characters long.");
        }
        
        /*
        int pos = path.find_last_of(L'\\');
        int size = path.size();
        if (pos != size - 1)
        {
            throw std::exception("CUtils::TraverseFS no trailing slash after path");
        }
        */

        wstringstream ss;
        BOOL bContinue = TRUE;
        WIN32_FIND_DATAW data;
        hFind = FindFirstFileW(path.c_str(), &data);
        if (INVALID_HANDLE_VALUE == hFind){
            jclass exceptionClass = env->FindClass("java/lang/Exception");
            env->ThrowNew(exceptionClass, "FindFirstFileW returned invalid handle.");
        }


        //HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        //DWORD dwBytesWritten;


        // If we have no error, loop thru the files in this dir
        while (hFind && bContinue){

          /*
          //Debug Print Statment. DO NOT DELETE! cout and wcout do not print unicode correctly.
            WriteConsole(hStdOut, data.cFileName, (DWORD)_tcslen(data.cFileName), &dwBytesWritten, NULL);
            WriteConsole(hStdOut, L"\n", 1, &dwBytesWritten, NULL);
            */

          //Check if this entry is a directory
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                // Make sure this dir is not . or ..
                if (wstring(data.cFileName) != L"." &&
                    wstring(data.cFileName) != L"..")
                {   
                    ss << wstring(data.cFileName) << L"\\" << L"\n";
                }
            }
            else{
                ss << wstring(data.cFileName) << L"\n";
            }
            bContinue = FindNextFileW(hFind, &data);
        }   
        FindClose(hFind); // Free the dir structure



        wstring cstr = ss.str();
        int len = cstr.size();
        //WriteConsole(hStdOut, cstr.c_str(), len, &dwBytesWritten, NULL);
        //WriteConsole(hStdOut, L"\n", 1, &dwBytesWritten, NULL);
        jchar* raw = new jchar[len];
        memcpy(raw, cstr.c_str(), len*sizeof(wchar_t));
        jstring result = env->NewString(raw, len);
        delete[] raw;
        return result;
    }
    catch(...){
        FindClose(hFind);
        jclass exceptionClass = env->FindClass("java/lang/Exception");
        env->ThrowNew(exceptionClass, "Exception occured.");
    }
    
    return NULL;
}

 /* 
 //Code used to return a list of drives. Does not include disconnected network drives...
 DWORD dwLogicalDrives = GetLogicalDrives();
 UINT nDrive = 0;
 for ( nDrive = 0; nDrive<32; nDrive++ ){
     if ( dwLogicalDrives & (1 << nDrive) ){
         wchar_t arr[] = { nDrive+'A', ':', '\\' }; 
         wstring drive;
         drive.assign(arr, arr + 3);
         UINT uType = GetDriveTypeW(drive.c_str());
         wstring type = L"";
         switch(uType){
            case DRIVE_REMOVABLE: type = L"FLOPPY"; break;
            case DRIVE_FIXED:     type = L"FIXED"; break;
            case DRIVE_REMOTE:    type = L"NETWORK"; break;
            case DRIVE_CDROM:     type = L"CDROM"; break;
            case DRIVE_RAMDISK:   type = L"RAMDISK"; break;
         }
         ss << drive << L"\t" << type << L"\n";
     }
 }
 */