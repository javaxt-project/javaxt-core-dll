#include <jni.h>
#include <stdlib.h>
#include <string.h> 


/* Header for class utils_ntfs_FileSystemWatcherNative */

#ifndef _Included_javaxt_io_FileSystemWatcherNative
#define _Included_javaxt_io_FileSystemWatcherNative
#ifdef __cplusplus
extern "C" {
#endif
#undef utils_ntfs_FileSystemWatcherNative_INFINITE
#define utils_ntfs_FileSystemWatcherNative_INFINITE -1L
#undef utils_ntfs_FileSystemWatcherNative_WAIT_FAILED
#define utils_ntfs_FileSystemWatcherNative_WAIT_FAILED -1L
#undef utils_ntfs_FileSystemWatcherNative_WAIT_ABANDONED
#define utils_ntfs_FileSystemWatcherNative_WAIT_ABANDONED 128L
#undef utils_ntfs_FileSystemWatcherNative_WAIT_OBJECT_0
#define utils_ntfs_FileSystemWatcherNative_WAIT_OBJECT_0 0L
#undef utils_ntfs_FileSystemWatcherNative_WAIT_TIMEOUT
#define utils_ntfs_FileSystemWatcherNative_WAIT_TIMEOUT 258L






//**************************************************************************
//** FindFirstChangeNotification
//**************************************************************************	
/*
 * Class:     utils_ntfs_FileSystemWatcherNative
 * Method:    FindFirstChangeNotification
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jlong JNICALL Java_javaxt_io_FileSystemWatcherNative_FindFirstChangeNotification
  (JNIEnv *, jclass, jstring, jboolean, jint);


//**************************************************************************
//** FindNextChangeNotification
//**************************************************************************
/*
 * Class:     utils_ntfs_FileSystemWatcherNative
 * Method:    FindNextChangeNotification
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_javaxt_io_FileSystemWatcherNative_FindNextChangeNotification
  (JNIEnv *, jclass, jlong);


//**************************************************************************
//** FindCloseChangeNotification
//**************************************************************************
/*
 * Class:     utils_ntfs_FileSystemWatcherNative
 * Method:    FindCloseChangeNotification
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_javaxt_io_FileSystemWatcherNative_FindCloseChangeNotification
  (JNIEnv *, jclass, jlong);


//**************************************************************************
//** WaitForSingleObject
//**************************************************************************
/*
 * Class:     utils_ntfs_FileSystemWatcherNative
 * Method:    WaitForSingleObject
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_javaxt_io_FileSystemWatcherNative_WaitForSingleObject
  (JNIEnv *, jclass, jlong, jint);


//**************************************************************************
//** ReadDirectoryChangesW
//**************************************************************************
/*
 * Class:     utils_ntfs_FileSystemWatcherNative
 * Method:    ReadDirectoryChangesW
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_FileSystemWatcherNative_ReadDirectoryChangesW
  (JNIEnv *, jclass);



#ifdef __cplusplus
}
#endif
#endif
