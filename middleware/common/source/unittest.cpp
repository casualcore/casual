//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/signal.h"
#include "common/execution.h"

#include "common/message/event.h"
#include "common/message/handle.h"

#include "common/exception/casual.h"

// std
#include <random>

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         namespace clean
         {
            Scope::Scope() 
            { 
               execution::reset();
               signal::clear();
            }

            Scope::~Scope() { signal::clear();}

         } // clean

         namespace local
         {
            namespace
            {
               size_type transform_size( size_type size)
               {
                  const size_type size_type_size = sizeof( platform::binary::type::size_type);

                  if( size < size_type_size)
                  {
                     throw exception::system::invalid::Argument{ "mockup message size is to small"};
                  }
                  return size - size_type_size;
               }
            } // <unnamed>
         } // local

         Message::Message() = default;
         Message::Message( size_type size) : payload( local::transform_size( size)) {}

         size_type Message::size() const { return payload.size() + sizeof( platform::binary::type::size_type);}

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

            platform::binary::type::value_type byte()
            {
               auto distribution = local::distribution();

               return distribution( local::engine());
            }

            platform::binary::type binary( size_type size)
            {
               platform::binary::type result( size);

               local::randomize( result);

               return result;
            }

            unittest::Message message( size_type size)
            {
               unittest::Message result( size);

               local::randomize( result.payload);
               return result;
            }
         } // random

         namespace domain
         {
            namespace manager
            {

               void wait( communication::ipc::inbound::Device& device)
               {
                  struct wait_done{};


                  auto handler = device.handler(
                     []( const message::event::domain::boot::End&){
                        throw wait_done{};
                     },
                     []( const message::event::domain::Error& error){
                        if( error.severity == message::event::domain::Error::Severity::fatal)
                        {
                           throw exception::casual::Shutdown{ string::compose( "fatal error: ", error)};
                        }
                     },
                     common::message::handle::Discard< common::message::event::domain::Group>{},
                     common::message::handle::Discard< common::message::event::domain::boot::Begin>{},
                     common::message::handle::Discard< common::message::event::domain::shutdown::Begin>{},
                     common::message::handle::Discard< common::message::event::domain::shutdown::End>{},
                     common::message::handle::Discard< common::message::event::domain::server::Connect>{},
                     common::message::handle::Discard< common::message::event::process::Spawn>{},
                     common::message::handle::Discard< common::message::event::process::Exit>{}
                  );

                  try
                  {
                     message::dispatch::blocking::pump( handler, device);
                  }
                  catch( const wait_done&)
                  {
                     log::line( log::debug, "domain manager booted");
                     // no-op
                  }
               }

            } // manager

         } // domain
      } // unittest
   } // common
} // casual

