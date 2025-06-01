//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "service/conversation/state.h"

#include "common/flag/service/conversation.h"
#include "common/buffer/type.h"

#include "casual/xatmi/defines.h"

namespace casual
{
   namespace exception
   {
      namespace conversation
      {
         using Event = common::flag::service::conversation::Event;

      } // conversation

   } // exception

   namespace service::conversation
   {
      using Event = common::flag::service::conversation::Event;

      namespace connect
      {
         using Flag = common::flag::service::conversation::connect::Flag;
      } // connect

      namespace send
      {
         using Flag = common::flag::service::conversation::send::Flag;

         struct Result
         {
            Event event{};
            long user = 0;
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( event);
               CASUAL_SERIALIZE( user);
            )
         };
      } // send

      namespace receive
      {
         using Flag = common::flag::service::conversation::receive::Flag;

         struct Result
         {
            Event event{};
            long user = 0;
            common::buffer::Payload buffer;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( event);
               CASUAL_SERIALIZE( user);
               CASUAL_SERIALIZE( buffer);
            )
         };
      } // receive

      class Context
      {
      public:
         static Context& instance();
         ~Context();

         common::strong::conversation::descriptor::id connect( const std::string& service, common::buffer::payload::Send buffer, connect::Flag flags);

         send::Result send( common::strong::conversation::descriptor::id descriptor, common::buffer::payload::Send&& buffer, send::Flag flags);

         receive::Result receive( common::strong::conversation::descriptor::id descriptor, receive::Flag flags);

         void disconnect( common::strong::conversation::descriptor::id descriptor);

         inline auto& descriptors() { return m_state.descriptors;}

         bool pending() const;

      private:
         Context();

         State m_state;

      };

      inline Context& context() { return Context::instance();}

   } // service::conversation

} // casual


