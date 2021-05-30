#include "helper.h"
#include <string>
#include <vector>

class HelperLocationContext
{
public:
   int x;
};

class HelperContext
{
public:
   int state;
   int number_of_calls;
   int number_of_receives;
   std::string service;
   std::vector< char> input;
   std::vector< char> output;
   std::string content_type;
};

static_assert( sizeof( HelperLocationContext) <= sizeof( helper_location_ctx_t::memory), "sizeof ( helper_location_ctx_t::memory ) must be equal or greater than sizeof (helper_location_ctx_t::memory)" );
static_assert( sizeof( HelperContext) <= sizeof( helper_ctx_t::memory), "sizeof ( helper_ctx_t::memory ) must be equal or greater than sizeof (helper_ctx_t::memory)" );

extern int helper_init( helper_location_ctx_t *ctx)
{
   HelperLocationContext *context = reinterpret_cast< HelperLocationContext*>( ctx->pImpl);
   if ( context != nullptr)
   {
      return HELPER_ERROR;
   }

   context = new( ctx->memory) HelperLocationContext;
   ctx->pImpl = context;
   return HELPER_SUCCESS;
}

extern void helper_exit( helper_location_ctx_t *ctx)
{
   HelperLocationContext *context = reinterpret_cast< HelperLocationContext*>( ctx->pImpl);
   if ( context != nullptr)
   {
      context->~HelperLocationContext();
   }
}

extern int helper_call( helper_ctx_t *ctx)
{
   if ( ctx->pImpl == nullptr)
   {
      ctx->pImpl = new( ctx->memory) HelperContext;
   }
   HelperContext *context = reinterpret_cast< HelperContext*>( ctx->pImpl);
   if ( context == nullptr)
   {
      return HELPER_ERROR;
   }

   if ( ++context->number_of_calls < 3)
   {
      return HELPER_AGAIN;
   }
   else
   {
      return HELPER_SUCCESS;
   }
}

extern int helper_receive( helper_ctx_t *ctx)
{
    HelperContext *context = reinterpret_cast< HelperContext*>( ctx->pImpl);
    if ( context == nullptr)
    {
        return HELPER_ERROR;
    }

    if ( ++context->number_of_receives < 3)
    {
        return HELPER_AGAIN;
    }

   auto &data = context->output;

   // create response data...
   // char message[] = "response data";
   // data.insert( std::begin( data), &message[ 0], &message[ 0] + sizeof message - 1);
   data.insert( std::begin( data), std::begin( context->input), std::end( context->input)); // echo

   ctx->content = data.data();
   ctx->content_length = data.size();

   context->content_type = "plain/text";
   ctx->content_type.data = (char*)context->content_type.data();
   ctx->content_type.len = context->content_type.size();

   ctx->response_status = 200;

   return HELPER_SUCCESS;
}

extern void helper_cleanup( helper_ctx_t *ctx)
{
   HelperContext *context = reinterpret_cast< HelperContext*>( ctx->pImpl);
   if ( context != nullptr)
   {
      context->~HelperContext();
   }
}

extern int helper_push_buffer( helper_ctx_t *ctx, const char *data, const char *end)
{
   if ( ctx->pImpl == nullptr)
   {
      ctx->pImpl = new( ctx->memory) HelperContext;
   }
   HelperContext *context = reinterpret_cast< HelperContext*>( ctx->pImpl);
   if ( context == nullptr)
   {
      return HELPER_ERROR;
   }

   context->input.insert( std::end( context->input), data, end);
   return HELPER_SUCCESS;
}
