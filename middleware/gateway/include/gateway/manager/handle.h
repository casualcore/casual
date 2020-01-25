//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "gateway/manager/state.h"
#include "gateway/message.h"

#include "common/message/dispatch.h"
#include "common/message/domain.h"
#include "common/message/gateway.h"
#include "common/message/coordinate.h"

#include "common/message/event.h"

#include "common/communication/ipc.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace ipc
         {
            common::communication::ipc::inbound::Device& device();
         } // ipc


         namespace handle
         {

            void shutdown( State& state);

            void boot( State& state);

            struct Base
            {
               inline Base( State& state) : m_state( state) {}
            
               inline State& state() { return m_state.get();};
               inline const State& state() const { return m_state.get();};

            private:
               std::reference_wrapper< State> m_state;
            };



            namespace process
            {

               void exit( const common::process::lifetime::Exit& exit);

               struct Exit : Base
               {
                  using Base::Base;
                  using message_type = common::message::event::process::Exit;

                  void operator () ( message_type& message);

               };
            } // process

            namespace domain
            {
               namespace discover
               {
                  struct Request : Base
                  {
                     using Base::Base;
                     using message_type = common::message::gateway::domain::discover::Request;

                     void operator () ( message_type& message);
                  };

                  struct Reply : Base
                  {
                     using Base::Base;
                     using message_type = common::message::gateway::domain::discover::Reply;

                     void operator () ( message_type& message);
                  };

               } // discovery
            } // domain

            namespace outbound
            {
               namespace configuration
               {
                  struct Request : Base
                  {
                     using Base::Base;
                     using message_type = message::outbound::configuration::Request;

                     void operator () ( message_type& message);
                  };

               } // configuration

               struct Connect : Base
               {
                  using Base::Base;
                  using message_type = message::outbound::Connect;

                  void operator () ( message_type& message);

               };

            } // inbound

            namespace inbound
            {
               struct Connect : Base
               {
                  using Base::Base;
                  using message_type = message::inbound::Connect;

                  void operator () ( message_type& message);

               };

            } // inbound

            namespace listen
            {
               struct Accept : Base
               {
                  using Base::Base;

                  using descriptor_type = common::strong::file::descriptor::id;

                  void read( descriptor_type descriptor);
                  const std::vector< descriptor_type>& descriptors() const;
               };
            } // listen
         } // handle


         common::communication::ipc::dispatch::Handler handler( State& state);

      } // manager

   } // gateway


} // casual


