//
// casual_string_buffer.h
//
//  Created on: 22 okt 2013
//      Author: Kristone
//

#ifndef CASUAL_STRING_BUFFER_H_
#define CASUAL_STRING_BUFFER_H_

/* used as type with tpalloc */
#define CASUAL_STRING "CSTRING"

#define CASUAL_STRING_SUCCESS 0
#define CASUAL_STRING_INVALID_HANDLE 1
#define CASUAL_STRING_INVALID_ARGUMENT 2
#define CASUAL_STRING_OUT_OF_BOUNDS 3
#define CASUAL_STRING_OUT_OF_MEMORY 4
#define CASUAL_STRING_INTERNAL_FAILURE 9


#ifdef __cplusplus
extern "C" {
#endif

const char* casual_string_description( int code);

int casual_string_explore_buffer( const char* buffer, long* size, long* used);

int casual_string_write( char** buffer, const char* value);
int casual_string_parse( const char* buffer, const char** value);


#ifdef __cplusplus
}
#endif

#endif /* CASUAL_STRING_BUFFER_H_ */
