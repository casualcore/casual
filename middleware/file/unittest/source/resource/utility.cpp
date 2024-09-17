//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/resource/unittest/utility.h"

#include "file/manager/admin/services.h"

#include "file/message.h"
#include "file/factory.h"
#include "file/instance.h"

#include "serviceframework/service/protocol/call.h"

namespace casual
{
   namespace file::resource::unittest
   {
      manager::admin::model::State state()
      {
         serviceframework::service::protocol::binary::Call call;
         auto reply = call( manager::admin::service::name::state);

         decltype( state()) result;
         reply >> CASUAL_NAMED_VALUE( result);
         return result;
      }

      namespace blocking
      {
         namespace reserve
         {
            namespace local
            {
               namespace
               {
                  namespace device
                  {
                     auto& outbound()
                     {
                        static common::communication::instance::outbound::detail::optional::Device singleton{ instance::identity};
                        return singleton;
                     }

                     auto& inbound()
                     {
                        return common::communication::ipc::inbound::device();
                     }
                  } // device
               } // 
            } // local

            void send( std::filesystem::path path)
            {
               common::communication::device::blocking::send( 
                  local::device::outbound(),
                  factory::create::blocking::request( std::move( path)));
            }

            std::filesystem::path receive()
            {
               message::reserve::Reply reply;
               common::communication::device::blocking::receive( 
                  local::device::inbound(), 
                  reply);
               return std::move( reply.path);
            }
         } // reserve

      } // blocking

   } // file::resource::unittest
} // casual
