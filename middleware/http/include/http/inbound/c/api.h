//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#ifdef EXPORT
#define API __attribute__((visibility ("default")))
#else
#define API
#endif


#include <stddef.h> // size_t

// c-structs to carry information from nginx to casual and back
// headers
typedef struct casual_http_inbound_header_s
{
   char* key;
   char* value;
} casual_http_inbound_header_t;

typedef struct casual_http_inbound_headers_s
{
   casual_http_inbound_header_t* data;
   size_t size;
} casual_http_inbound_headers_t;

// string type
typedef struct casual_http_inbound_string_s
{
   char* data;
   size_t size;
} casual_http_inbound_string_t;

// request from nginx to casual
typedef struct casual_http_inbound_request_s
{
   casual_http_inbound_string_t method;
   casual_http_inbound_string_t url;
   casual_http_inbound_string_t service;
   casual_http_inbound_headers_t headers;
} casual_http_inbound_request_t;

// reply from casual to nginx
typedef struct casual_http_inbound_reply_s
{
   int code;
   casual_http_inbound_string_t content_type;
   casual_http_inbound_string_t payload;
   casual_http_inbound_headers_t headers;
} casual_http_inbound_reply_t;

// type to contain context both to casual-context and to used file descriptor
typedef struct casual_http_inbound_handle_s
{
    int fd; // triggered when it's time to poll (when fd is "readable")
    void* context_holder; // containing c++ context
} casual_http_inbound_handle_t;


enum Cycle { done = 0, again = -2};
enum Directive { service = 0, forward = 1};

// c-api
extern API void casual_http_inbound_request_set( casual_http_inbound_handle_t* handle, casual_http_inbound_request_t* request);
extern API void casual_http_inbound_reply_get( casual_http_inbound_handle_t* handle, casual_http_inbound_reply_t* reply);
extern API void casual_http_inbound_push_payload(casual_http_inbound_handle_t* handle, const unsigned char* ptr, size_t size);
extern API void casual_http_inbound_call(casual_http_inbound_handle_t* handle, enum Directive directive);
extern API int casual_http_inbound_receive(casual_http_inbound_handle_t* handle);
extern API void casual_http_inbound_deallocate( casual_http_inbound_handle_t* handle);


#ifdef __cplusplus
}
#endif
