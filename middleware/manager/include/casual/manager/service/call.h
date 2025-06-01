//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/service.h"
#include "common/communication/device.h"
#include "common/communication/ipc.h"
#include "common/serialize/binary.h"

#include <string>

namespace casual
{
   namespace manager::service
   {
      // we don't necessarily have access to service-manager when doing
      // service calls.
      // To be able to get state and such from managers we call
      // natively and do our serialization "by hand".
      // 
      // not that much code, hence I think it's worth it to be able to 
      // have a somewhat sound dependency graph.

         
      template< typename R, typename D, typename... Ts> 
      R call( D&& device, std::string_view service, Ts&&... arguments)
      {
         auto correlation = [&]()
         {
            common::message::service::call::callee::Request request;
            request.process = common::process::handle();
            request.service.name = std::string{ service};
            request.buffer.type = common::buffer::type::binary;

            auto archive = common::serialize::binary::writer();
            (void)( archive << ... << arguments); // cast to void to point out we don't use the returned "this" from archives << operator.
            archive.consume( request.buffer.data);

            return common::communication::device::blocking::send( device, request);
         }();

         auto reply = common::communication::ipc::receive< common::message::service::call::Reply>( correlation);
         auto archive = common::serialize::binary::reader( reply.buffer.data);

         R result;
         archive >> result;

         return result;
      }
      
   } // manager::service
   
} // casual
