//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/binary.h"
#include "common/process.h"
#include "common/communication/instance.h"
#include "common/message/service.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace unittest
         {

            // we don't have access to service-manager (or other managers)
            // when we build domain-manager.
            // To be able to get state and such from domain-manager we call
            // natively and do our serialization "by hand".
            // 
            // not that much code, hence I think it's worth it to be able to 
            // test stuff locally within domain-manager.

            namespace detail
            {
               // centinel
               template< typename A>
               void serialize( A& archive) {}

               template< typename A, typename T, typename... Ts>
               void serialize( A& archive, T&& value, Ts&&... ts)
               {
                  archive << CASUAL_NAMED_VALUE( std::forward< T>( value));
                  serialize( archive, std::forward< Ts>( ts)...);
               }
            } // detail
               
            template< typename R, typename... Ts> 
            R call( std::string service, Ts&&... arguments)
            {
               auto correlation = [&]()
               {
                  common::message::service::call::callee::Request request;
                  request.process = common::process::handle();
                  request.service.name = std::move( service);
                  request.buffer.type = common::buffer::type::binary();

                  auto archive = common::serialize::binary::writer();
                  detail::serialize( archive, std::forward< Ts>( arguments)...);
                  archive.consume( request.buffer.memory);

                  return common::communication::device::blocking::send( common::communication::instance::outbound::domain::manager::device(), request);
               }();


               common::message::service::call::Reply reply;
               common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply, correlation);
               auto archive = common::serialize::binary::reader( reply.buffer.memory);

               R result;
               archive >> CASUAL_NAMED_VALUE( result);

               return result;
            }

         } // unittest
      } // manager
   } // domain
} // casual