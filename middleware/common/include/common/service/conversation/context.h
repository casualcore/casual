//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_

#include "common/service/conversation/state.h"
#include "common/service/conversation/flags.h"

#include "common/buffer/type.h"
#include "common/flag.h"

#include "xatmi.h"

namespace casual
{

   namespace common
   {
      namespace service
      {
         namespace conversation
         {


            namespace receive
            {
               struct Result
               {
                  common::Flags< Event> event;
                  common::buffer::Payload buffer;
               };
            } // receive

            class Context
            {
            public:
               static Context& instance();

               descriptor::type connect( const std::string& service, common::buffer::payload::Send buffer, connect::Flags flags);



               common::Flags< Event> send( descriptor::type descriptor, common::buffer::payload::Send&& buffer, common::Flags< send::Flag> flags);

               receive::Result receive( descriptor::type descriptor, common::Flags< receive::Flag> flags);

               inline State::holder_type& descriptors() { return m_state.descriptors;}

               bool pending() const;

            private:
               Context();

               State m_state;

            };

         } // conversation

      } // service
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_
