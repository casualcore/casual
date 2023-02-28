//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/message/service.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"


// std
#include <random>

namespace casual
{
   namespace common::unittest
   {

      Message::Message() = default;
      
      Message::Message( platform::binary::type payload) 
         : payload{ std::move( payload)} {}

      Message::Message( platform::size::type size) 
      : Message{ random::binary( size)} {}

      namespace message::transport
      {
         unittest::Message size( platform::size::type size)
         {
            constexpr platform::size::type overhead = sizeof( platform::size::type) + sizeof( Uuid::uuid_type);
            assert( size >= overhead);

            return { size - overhead};
         }
         
      } // message::transport

      namespace service
      {
         strong::correlation::id send( std::string service, platform::binary::type payload)
         {
            {
               common::message::service::lookup::Request request{ process::handle()};
               request.requested = std::move( service);
               request.context.semantic = decltype( request.context.semantic)::no_busy_intermediate;
               communication::device::blocking::send( communication::instance::outbound::service::manager::device(), request);
            }
            
            auto lookup = communication::ipc::receive< common::message::service::lookup::Reply>();
            
            if( lookup.state != decltype( lookup.state)::idle)
               code::raise::error( code::xatmi::no_entry);

            common::message::service::call::callee::Request message{ process::handle()};
            message.service = std::move( lookup.service);
            message.buffer.data = std::move( payload);
            message.buffer.type = common::buffer::type::x_octet;

            return communication::device::blocking::send( lookup.process.ipc, message);
         }

         namespace wait::until
         {
            void advertised( std::string_view service)
            {
               common::message::service::lookup::Request lookup_request{ process::handle()};
               lookup_request.requested = service;
               lookup_request.context.semantic = decltype( lookup_request.context.semantic)::wait;
               auto correlation = communication::device::blocking::send( communication::instance::outbound::service::manager::device(), lookup_request);
               communication::ipc::receive< common::message::service::lookup::Reply>( correlation);

               common::message::service::lookup::discard::Request discard_request{ process::handle()};
               discard_request.correlation = correlation;
               discard_request.requested = service;
               discard_request.reply = false;
               communication::device::blocking::send( communication::instance::outbound::service::manager::device(), discard_request);
            }
         } // wait::until

      } // service


      namespace random
      {

         namespace local
         {
            namespace
            {
               std::mt19937& engine()
               {
                  //static std::random_device device;
                  static std::mt19937 engine{ std::random_device{}()};
                  return engine;
               }

               auto distribution()
               {
                  using limit_type = std::numeric_limits< platform::binary::type::value_type>;

                  std::uniform_int_distribution<> distribution( limit_type::min(), limit_type::max());

                  return distribution;
               }

               template< typename C>
               void randomize( C& container)
               {
                  auto dist = local::distribution();

                  for( auto& value : container)
                  {
                     value = dist( local::engine());
                  }
               }

            } // <unnamed>
         } // local

         namespace detail
         {
            long long integer()
            {
               std::uniform_int_distribution< long long> distribution( std::numeric_limits< long long>::min(), std::numeric_limits< long long>::max());
               return distribution( local::engine());
            }
         } // detail

         platform::binary::type::value_type byte()
         {
            auto distribution = local::distribution();

            return distribution( local::engine());
         }

         platform::binary::type binary( platform::size::type size)
         {
            platform::binary::type result( size);

            local::randomize( result);

            return result;
         }

         std::string string( platform::size::type size)
         {
            std::string result( size, '\0');

            std::uniform_int_distribution< short> distribution( 32, 123);

            for( auto& value : result)
               value = distribution( local::engine());

            return result;
         }

      } // random

   } // common::unittest
} // casual

