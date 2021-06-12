/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#include <stdarg.h>
#include <uuid/uuid.h>


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

extern void casual_execution_id_set( const uuid_t* id);
extern const uuid_t* casual_execution_id_get();

/**
 * @returns the alias of the instance.
 * 
 * @attention could be NULL if the instance is *not* spawn by
 * casual-domain-manager
 */
extern const char* casual_instance_alias();

/**
 * @returns the instance index. 
 * Example: if a server is configured with 3 instances.
 * each instance will have 0, 1, and 2, respectively.  
 * 
 * @attention could return -1 if the instance is *not* spawn by
 * casual-domain-manager
 */
extern long casual_instance_index();

/**
 * properties for a given service given to the callback during browsing
 */
struct casual_browsed_service
{
   const char* name;
};

/**
 *  callback declaration for casual_instance_browse_services
 */
typedef int( *casual_instance_browse_callback)( const casual_browsed_service* service, void* context);

/**
 * Browse all services that this instance has advertised, either automatically via 
 * what the server is build with (casual-build-server) and/or what this instance
 * has advertised explicitly via `tpadvertise`
 * 
 * The suplied `context` will be included in all callback calls, hence user can keep state.
 * 
 * Will browse all services. If `callback` returns other than `0`, the browsing will
 * stop.
 */
extern void casual_instance_browse_services( casual_instance_browse_callback callback, void* context);


#ifdef __cplusplus
}
#endif


