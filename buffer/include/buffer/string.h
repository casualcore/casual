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

#define CASUAL_STRING_NO_SPACE 1

#define CASUAL_STRING_NO_PLACE 2

#define CASUAL_STRING_INVALID_BUFFER 3

#define CASUAL_STRING_INVALID_ARGUMENT 4

#define CASUAL_STRING_INTERNAL_FAILURE 9


#ifdef __cplusplus
extern "C" {
#endif

const char* CasualStringDescription( int code);

int CasualStringExploreBuffer( const char* buffer, long* size, long* used);

int CasualStringWriteString( char* buffer, const char* value);
int CasualStringParseString( const char* buffer, const char** value);


#ifdef __cplusplus
}
#endif

#endif /* CASUAL_STRING_BUFFER_H_ */
