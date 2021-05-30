#ifndef HELPER_H
#define HELPER_H 1
#ifdef __cplusplus
extern "C" {
#endif

#ifdef EXPORT
#define API __attribute__((visibility ("default")))
#else
#define API
#endif

#define HELPER_SUCCESS 0
#define HELPER_ERROR -1
#define HELPER_AGAIN -2

typedef struct helper_location_ctx
{
    void *pImpl;
    char memory[16*sizeof(void*)];
} helper_location_ctx_t;

typedef struct helper_ctx
{
    char *content;
    long content_length;
    struct
    {
        long len;
        char *data;
    } content_type;
    int response_status;

    void *pImpl;
    char memory[16*sizeof(void*)];
} helper_ctx_t;

extern API int helper_init(helper_location_ctx_t *ctx);
extern API void helper_exit(helper_location_ctx_t *ctx);
extern API int helper_call(helper_ctx_t *ctx);
extern API int helper_receive(helper_ctx_t *ctx);
extern API void helper_cleanup(helper_ctx_t *ctx);
extern API int helper_push_buffer(helper_ctx_t *ctx, const char *data, const char *end);

#ifdef __cplusplus
}
#endif
#endif
