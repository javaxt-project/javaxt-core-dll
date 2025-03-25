#include <windows.h>
#include <stdio.h>

//******************************************************************************
//**  Junction Utilities
//******************************************************************************
/**
 *   Functions used to create and analyze Windows Junction Points.
 *   Contains code by Inv Softworks LLC, www.flexhex.com.
 *   http://www.flexhex.com/docs/articles/hard-links.phtml
 *
 ******************************************************************************/


#define REPARSE_MOUNTPOINT_HEADER_SIZE   8

typedef struct {
  DWORD ReparseTag;
  DWORD ReparseDataLength;
  WORD Reserved;
  WORD ReparseTargetLength;
  WORD ReparseTargetMaximumLength;
  WORD Reserved1;
  WCHAR ReparseTarget[1];
} REPARSE_MOUNTPOINT_DATA_BUFFER, *PREPARSE_MOUNTPOINT_DATA_BUFFER;

#define FSCTL_GET_REPARSE_POINT 0x000900a8
#define FSCTL_SET_REPARSE_POINT 0x000900a4

//**************************************************************************
//** OpenDirectory
//**************************************************************************
/**  Used get a handle for a given path.
 */
HANDLE OpenDirectory(const wchar_t* path, BOOL bReadWrite) {

  //Obtain restore privilege in case we don't have it
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    ::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    ::LookupPrivilegeValue(NULL,
                         (bReadWrite ? SE_RESTORE_NAME : SE_BACKUP_NAME),
                         &tp.Privileges[0].Luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    ::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    ::CloseHandle(hToken);

  //Open the directory
    DWORD dwAccess = bReadWrite ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
    HANDLE hDir = ::CreateFileW(path, dwAccess, 0, NULL, OPEN_EXISTING,
                     FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);

    return hDir;
}


#define DIR_ATTR  (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)


//**************************************************************************
//** IsDirectoryJunction
//**************************************************************************
/**  Used to determine whether a given path represents a Junction point.
 */
BOOL IsDirectoryJunction(const wchar_t* path) {
    DWORD dwAttr = ::GetFileAttributesW(path);
    if (dwAttr == -1) return FALSE;  // Not exists
    if ((dwAttr & DIR_ATTR) != DIR_ATTR) return FALSE;  // Not dir or no reparse point

    HANDLE hDir = OpenDirectory(path, FALSE);
    if (hDir == INVALID_HANDLE_VALUE) return FALSE;  // Failed to open directory

    BYTE buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    REPARSE_MOUNTPOINT_DATA_BUFFER& ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER&)buf;
    DWORD dwRet;
    BOOL br = ::DeviceIoControl(hDir, FSCTL_GET_REPARSE_POINT, NULL, 0, &ReparseBuffer,
                                      MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwRet, NULL);
    ::CloseHandle(hDir);
    return br ? (ReparseBuffer.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) : FALSE;
}


//**************************************************************************
//** GetTarget
//**************************************************************************
/**  Used to return the path for a given junction.
 */
LPCWSTR GetTarget(const wchar_t* path) {


    if (!IsDirectoryJunction(path)) {
        return NULL; // Throw Error: no junction here
    }

    // Open for reading only (see OpenDirectory definition above)
    HANDLE hDir = OpenDirectory(path, FALSE);

    BYTE buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];  // We need a large buffer
    REPARSE_MOUNTPOINT_DATA_BUFFER& ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER&)buf;
    DWORD dwRet;


    if (::DeviceIoControl(hDir, FSCTL_GET_REPARSE_POINT, NULL, 0, &ReparseBuffer,
                                     MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwRet, NULL)) {
        // Success
        ::CloseHandle(hDir);

        LPCWSTR pPath = ReparseBuffer.ReparseTarget;
        if (wcsncmp(pPath, L"\\??\\", 4) == 0) pPath += 4;  // Skip 'non-parsed' prefix
        return pPath;

    }
    else {  // Error
      DWORD dr = ::GetLastError();
      ::CloseHandle(hDir);
      // Some error action (throw or MessageBox)
    }

    return NULL;
}


//**************************************************************************
//** CreateJunction
//**************************************************************************
/**  Used to create a junction point.
 *
static void CreateJunction(LPCSTR szJunction, LPCSTR szPath) {
    
    BYTE buf[sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
    REPARSE_MOUNTPOINT_DATA_BUFFER& ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER&)buf;
    char szTarget[MAX_PATH] = "\\??\\";

    strcat(szTarget, szPath);
    strcat(szTarget, "\\");

    if (!::CreateDirectory(szJunction, NULL)) throw ::GetLastError();

    // Obtain SE_RESTORE_NAME privilege (required for opening a directory)
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp;
    try {
        if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) throw ::GetLastError();
        if (!::LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &tp.Privileges[0].Luid))  throw ::GetLastError();
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))  throw ::GetLastError();
    }
    catch (DWORD) { }   // Ignore errors
    if (hToken) ::CloseHandle(hToken);

    HANDLE hDir = ::CreateFile(szJunction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hDir == INVALID_HANDLE_VALUE) throw ::GetLastError();

    memset(buf, 0, sizeof(buf));
    ReparseBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    int len = ::MultiByteToWideChar(CP_ACP, 0, szTarget, -1, ReparseBuffer.ReparseTarget, MAX_PATH);
    ReparseBuffer.ReparseTargetMaximumLength = (len--) * sizeof(WCHAR);
    ReparseBuffer.ReparseTargetLength = len * sizeof(WCHAR);
    ReparseBuffer.ReparseDataLength = ReparseBuffer.ReparseTargetLength + 12;

    DWORD dwRet;
    if (!::DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, &ReparseBuffer, ReparseBuffer.ReparseDataLength+REPARSE_MOUNTPOINT_HEADER_SIZE, NULL, 0, &dwRet, NULL)) {
        DWORD dr = ::GetLastError();
        ::CloseHandle(hDir);
        ::RemoveDirectory(szJunction);
        throw dr;
    }

    ::CloseHandle(hDir);
    
} // CreateJunction
*/