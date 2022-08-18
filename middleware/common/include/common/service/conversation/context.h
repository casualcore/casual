//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/service/conversation/state.h"
#include "common/flag/service/conversation.h"
#include "common/buffer/type.h"

#include "casual/xatmi/defines.h"

namespace casual
{

   namespace common
   {
      namespace exception
      {
         namespace conversation
         {
            struct Event
            {
               Event( flag::service::conversation::Events event) :
                  event( event) {}

               flag::service::conversation::Events event;
            };

         } // conversation

      } // exception

      namespace service
      {
         namespace conversation
         {
            using Event = flag::service::conversation::Event;
            using Events = flag::service::conversation::Events;

            namespace connect
            {
               using Flag = flag::service::conversation::connect::Flag;
               using Flags = flag::service::conversation::connect::Flags;
            } // connect

            namespace send
            {
               using Flag = flag::service::conversation::send::Flag;
               using Flags = flag::service::conversation::send::Flags;
               struct Result
               {
                  common::Flags< Event> event;
                  long user = 0;
                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( event);
                     CASUAL_SERIALIZE( user);
                  )
               };
            } // send

            namespace receive
            {
               using Flag = flag::service::conversation::receive::Flag;
               using Flags = flag::service::conversation::receive::Flags;

               struct Result
               {
                  common::Flags< Event> event;
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

               strong::conversation::descriptor::id connect( const std::string& service, common::buffer::payload::Send buffer, connect::Flags flags);

               send::Result send( strong::conversation::descriptor::id descriptor, common::buffer::payload::Send&& buffer, common::Flags< send::Flag> flags);

               receive::Result receive( strong::conversation::descriptor::id descriptor, common::Flags< receive::Flag> flags);

               void disconnect( strong::conversation::descriptor::id descriptor);

               inline auto& descriptors() { return m_state.descriptors;}

               bool pending() const;

            private:
               Context();

               State m_state;

            };

            inline Context& context() { return Context::instance();}


         } // conversation

      } // service
   } // common

} // casual


