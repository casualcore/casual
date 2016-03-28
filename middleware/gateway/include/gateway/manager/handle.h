//!
//! handle.h
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_HANDLE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_HANDLE_H_

#include "gateway/manager/state.h"
#include "gateway/message.h"

#include "common/message/dispatch.h"

#include "common/communication/ipc.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace ipc
         {
            const common::communication::ipc::Helper& device();
         } // ipc


         namespace handle
         {

            void shutdown( State& state);

            void boot( State& state);

            struct Base
            {
               Base( State& state);
            protected:
               State& state();
            private:
               std::reference_wrapper< State> m_state;
            };

            namespace listener
            {

               struct Event : Base
               {
                  using Base::Base;
                  using message_type = message::manager::listener::Event;

                  void operator () ( message_type& message);
               };

            } // listener

            namespace process
            {

               void exit( const common::process::lifetime::Exit& exit);

               struct Exit : Base
               {
                  using Base::Base;
                  using message_type = common::message::process::termination::Event;

                  void operator () ( message_type& message);

               };


            } // process

            namespace outbound
            {
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

               namespace ipc
               {
                  struct Connect : Base
                  {
                     using Base::Base;
                     using message_type = message::ipc::connect::Request;

                     void operator () ( message_type& message);

                  };

               } // ipc

               namespace tcp
               {
                  struct Connect : Base
                  {
                     using Base::Base;
                     using message_type = message::tcp::Connect;

                     void operator () ( message_type& message);

                  };
               } // tcp

            } // inbound

         } // handle

         common::message::dispatch::Handler handler( State& state);

      } // manager

   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_HANDLE_H_
