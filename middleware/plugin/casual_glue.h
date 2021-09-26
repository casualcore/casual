#ifndef CASUAL_GLUE_H
#define CASUAL_GLUE_H 1
#ifdef __cplusplus
extern "C" {
#endif


#ifdef EXPORT
#define API __attribute__((visibility ("default")))
#else
#define API
#endif


#include <stddef.h> // size_t

typedef struct CasualGlueHandle
{
    int fd; // triggered when it's time to poll (when fd is "readable")
    void *impl;
} casual_glue_handle_t;

typedef struct Input
{
    char *method;
    char *url;
    char *service;
    char *headers;
    // data is added to impl with casual_glue_push_chunk
} input_t;

typedef struct Output
{
    int status;
    unsigned char *data; // all data at once
    size_t size;
    unsigned char *headers;
} output_t;

extern API int casual_glue_start(casual_glue_handle_t *handle);
extern API int casual_glue_push_chunk(casual_glue_handle_t *handle, const unsigned char *ptr, size_t size);
extern API int casual_glue_call(casual_glue_handle_t *handle, input_t *input);
extern API int casual_glue_poll(casual_glue_handle_t *handle, output_t *output);
extern API int casual_glue_end(casual_glue_handle_t *handle);


#ifdef __cplusplus
}
#endif
#endif
