#include <jni.h>
#include "Junction.h"

/* Header for class javaxt_io_File */

#ifndef _Included_javaxt_io_File
#define _Included_javaxt_io_File
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     javaxt_io_File
 * Method:    GetFileAttributesEx
 * Signature: (Ljava/lang/String;)[J
 */
JNIEXPORT jlongArray JNICALL Java_javaxt_io_File_GetFileAttributesEx
  (JNIEnv *, jclass, jstring);



/*
 * Class:     javaxt_io_File
 * Method:    GetTarget
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_File_GetTarget
  (JNIEnv *, jclass, jstring);


/*
 * Class:     javaxt_io_File
 * Method:    GetSharedDrives
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_File_GetSharedDrives
  (JNIEnv *env, jclass, jstring);


/*
 * Class:     javaxt_io_File
 * Method:    GetNetworkDrives
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_File_GetNetworkDrives
  (JNIEnv *env, jclass);



/*
 * Class:     javaxt_io_File
 * Method:    GetFiles
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jstring JNICALL Java_javaxt_io_File_GetFiles
   (JNIEnv *env, jclass, jstring filename);




#ifdef __cplusplus
}
#endif
#endif