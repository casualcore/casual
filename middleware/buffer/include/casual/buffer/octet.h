/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once



/* used as type with tpalloc */
#define CASUAL_OCTET       "COCTET"
#define CASUAL_OCTET_JSON  "JSON"
#define CASUAL_OCTET_XML   "XML"
#define CASUAL_OCTET_YAML  "YAML"

#define CASUAL_OCTET_SUCCESS 0
#define CASUAL_OCTET_INVALID_HANDLE 1
#define CASUAL_OCTET_INVALID_ARGUMENT 2
#define CASUAL_OCTET_OUT_OF_MEMORY 4
#define CASUAL_OCTET_INTERNAL_FAILURE 128

const char* casual_octet_description( int code);


int casual_octet_explore_buffer( const char* buffer, const char** name, long* size);

int casual_octet_set( char** buffer, const char* data, long size);
int casual_octet_get( const char* buffer, const char** data, long* size);



