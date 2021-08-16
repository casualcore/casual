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


typedef struct HelperData
{
    void *pImpl;
} helper_data_t;

typedef struct HelperRequestData
{
    void *event_data;
    char *content;
    long content_length;
    struct
    {
        long len;
        char *data;
    } content_type;
    int response_status;

    void *pImpl;
} helper_request_data_t;

extern API int helper_init(helper_data_t *helper_data);
extern API void helper_exit(helper_data_t *helper_data);
extern API int helper_call(helper_data_t *helper_data, helper_request_data_t *helper_request_data);
extern API int helper_receive(helper_data_t *helper_data, helper_request_data_t *helper_request_data);
extern API void helper_cleanup(helper_request_data_t *helper_request_data);
extern API int helper_push_buffer(helper_request_data_t *helper_request_data, const char *data, const char *end);

#ifdef __cplusplus
}
#endif
#endif
