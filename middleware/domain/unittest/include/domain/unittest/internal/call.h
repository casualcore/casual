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
   namespace domain::unittest::internal
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
         template< typename A,  typename... Ts>
         void serialize( A& archive, Ts&&... ts)
         {
            (void)( archive << ... << ts);  // cast to void to point out we don't use the returned "this" from archives << operator.
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
            request.buffer.type = common::buffer::type::binary;

            auto archive = common::serialize::binary::writer();
            detail::serialize( archive, std::forward< Ts>( arguments)...);
            archive.consume( request.buffer.data);

            return common::communication::device::blocking::send( common::communication::instance::outbound::domain::manager::device(), request);
         }();


         common::message::service::call::Reply reply;
         common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply, correlation);
         auto archive = common::serialize::binary::reader( reply.buffer.data);

         R result;
         archive >> result;

         return result;
      }

   } // domain::unittest::internal
} // casual