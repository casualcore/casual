/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/* error codes
 */
#define CASUAL_FE_OK 0
#define CASUAL_FE_BUSY 1
#define CASUAL_FE_ERROR 2
#define CASUAL_FE_SIGNAL 3

/* @returns the error code associated last error */
int casual_file_last_error_code();

/* @returns the error text associated last error */
extern const char* casual_file_last_error_text();


/* @returns the reserved path if success and null on error */
extern char* casual_file_blocking_reserve( const char* path);

/* @returns the reserved path if success and null on error or if path is busy */
extern char* casual_file_non_blocking_reserve( const char* path);

/* releases the memory allocated during acquire or attempt */
extern void casual_file_release( char* path);

#ifdef __cplusplus
}
#endif
