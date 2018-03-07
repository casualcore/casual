//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_EXTENDED_H_
#define CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_EXTENDED_H_

#include <stdarg.h>





#define CASUAL_BUFFER_BINARY_TYPE ".binary"
#define CASUAL_BUFFER_BINARY_SUBTYPE 0
#define CASUAL_BUFFER_INI_TYPE ".ini"
#define CASUAL_BUFFER_INI_SUBTYPE 0
#define CASUAL_BUFFER_JSON_TYPE ".json"
#define CASUAL_BUFFER_JSON_SUBTYPE 0
#define CASUAL_BUFFER_XML_TYPE ".xml"
#define CASUAL_BUFFER_XML_SUBTYPE 0
#define CASUAL_BUFFER_YAML_TYPE ".yaml"
#define CASUAL_BUFFER_YAML_SUBTYPE 0


#ifdef __cplusplus
extern "C" {
#endif

extern void casual_service_forward( const char* service, char* data, long size);

typedef enum { c_log_error, c_log_warning, c_log_information, c_log_debug } casual_log_category_t;
extern int casual_log( casual_log_category_t category, const char* const format, ...);
extern int casual_vlog( casual_log_category_t category, const char* const format, va_list ap);

extern int casual_user_vlog( const char* category, const char* const format, va_list ap);

extern int casual_user_log( const char* category, const char* const message);

#ifdef __cplusplus
}
#endif

#endif // CASUAL_MIDDLEWARE_XATMI_INCLUDE_XATMI_EXTENDED_H_
