#include "helper.h"
#include "http/call.h"
#include <string>
#include <vector>

class HelperDataImpl
{
public:
   int x;
};

class HelperRequestDataImpl
{
public:
   HelperDataImpl *helperDataImpl = nullptr;
   casual::plugin::call::Context *user_data;
   std::string service;
   std::vector<char> input;
   std::vector<char> output;
   std::string content_type;
};


extern int helper_init(helper_data_t *helper_data)
{
   HelperDataImpl *helperDataImpl = new HelperDataImpl;
   helper_data->pImpl = helperDataImpl;
   return HELPER_SUCCESS;
}

extern void helper_exit(helper_data_t *helper_data)
{
   HelperDataImpl *helperDataImpl = reinterpret_cast<HelperDataImpl*>(helper_data->pImpl);
   delete helperDataImpl;
}


extern int helper_call(helper_data_t *helper_data, helper_request_data_t *helper_request_data)
{
   HelperRequestDataImpl *helperRequestDataImpl = nullptr;
   if (helper_request_data->pImpl == nullptr)
   {
      helperRequestDataImpl = new HelperRequestDataImpl;
      helper_request_data->pImpl = helperRequestDataImpl;
   }
   else
   {
      helperRequestDataImpl = reinterpret_cast<HelperRequestDataImpl*>(helper_request_data->pImpl);
   }
   if (helperRequestDataImpl->helperDataImpl == nullptr)
   {
      HelperDataImpl *helperDataImpl = reinterpret_cast<HelperDataImpl*>(helper_data->pImpl);
      helperRequestDataImpl->helperDataImpl = helperDataImpl;
   }

   casual::plugin::call::Arguments arguments{};
   arguments.service = helper_request_data->service;
   arguments.payload.header = helper_request_data.headers;
   arguments.payload.body = helperRequestDataImpl->input;
   helperRequestDataImpl->user_data = new casual::plugin::call::Context{ std::move(arguments)};

   return HELPER_SUCCESS;
}


extern int helper_receive(helper_data_t *helper_data, helper_request_data_t *helper_request_data)
{
   HelperRequestDataImpl *helperRequestDataImpl = reinterpret_cast<HelperRequestDataImpl*>(helper_request_data->pImpl);
   if (helperRequestDataImpl == nullptr)
   {
      return HELPER_ERROR;
   }
   
   // HelperDataImpl *helperDataImpl = reinterpret_cast<HelperDataImpl*>(helper_data->pImpl);

   std::optional<casual::plugin::call::Reply> reply;
   if (!(reply = helperRequestDataImpl->user_data->receive()))
   {
      return HELPER_AGAIN;
   }
   else
   {
      // set output data

      helperRequestDataImpl->output = reply.payload.body; // copy to storage tied to request

      helper_request_data->content = helperRequestDataImpl->output.data();
      helper_request_data->content_length = helperRequestDataImpl->output.size();

      helperRequestDataImpl->content_type = "plain/text"; // data must be stored somewhere
      helper_request_data->content_type.data = (char*)helperRequestDataImpl->content_type.data();
      helper_request_data->content_type.len = helperRequestDataImpl->content_type.size();

      helper_request_data->response_status = 200;

      return HELPER_SUCCESS;
   }
}

extern void helper_cleanup(helper_request_data_t *helper_request_data)
{
   HelperRequestDataImpl *helperRequestDataImpl = reinterpret_cast<HelperRequestDataImpl*>(helper_request_data->pImpl);
   if (helperRequestDataImpl != nullptr)
   {
      delete helperRequestDataImpl->user_data;
      delete helperRequestDataImpl;
   }
}

extern int helper_push_buffer(helper_request_data_t *helper_request_data, const char *data, const char *end)
{
   HelperRequestDataImpl *helperRequestDataImpl = nullptr;
   if (helper_request_data->pImpl == nullptr)
   {
      helperRequestDataImpl = new HelperRequestDataImpl;
      helper_request_data->pImpl = helperRequestDataImpl;
   }

   helperRequestDataImpl->input.insert(std::end(helperRequestDataImpl->input), data, end);
   return HELPER_SUCCESS;
}
