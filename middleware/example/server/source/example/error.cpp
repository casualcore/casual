//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/handle.h"
#include "common/server/handle/conversation.h"
#include "common/message/service.h"
#include "common/message/conversation.h"
#include "common/message/handle.h"
#include "common/log.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/instance.h"


namespace casual
{
   using namespace common;
   namespace example
   {
      namespace local
      {
         namespace
         {
            namespace error
            {
               namespace handle
               {
                  namespace service
                  {
                     namespace code
                     {
                        const std::map< std::string, std::function< message::service::Code()>> mapping{
                           { "casual/example/error/urcode", [](){ return message::service::Code{ common::code::xatmi::ok, 42};}},
                           { "casual/example/error/TPEOS", [](){ return message::service::Code{ common::code::xatmi::os, 0};}},
                           { "casual/example/error/TPEPROTO", [](){ return message::service::Code{ common::code::xatmi::protocol, 0};}},
                           { "casual/example/error/TPESVCERR", [](){ return message::service::Code{ common::code::xatmi::service_error, 0};}},
                           { "casual/example/error/TPESVCFAIL", [](){ return message::service::Code{ common::code::xatmi::service_fail, 0};}},
                           { "casual/example/error/TPESYSTEM", [](){ return message::service::Code{ common::code::xatmi::system, 0};}},
                        };

                        message::service::Code get( const std::string& service)
                        {
                           auto found = algorithm::find( mapping, service);

                           if( found)
                              return found->second();

                           return {};
                        }
                     } // code

                     namespace transform
                     {
                        auto ack = []( auto& request)
                        {
                           message::service::call::ACK ack;
                           ack.metric.start = platform::time::clock::type::now();
                           ack.metric.service = request.service.name;
                           ack.metric.parent = request.parent;
                           ack.metric.process = process::handle();
                           return ack;
                        };
                     } // transform

                     struct Call 
                     {
                        void operator() ( message::service::call::callee::Request& request)
                        {
                           
                           auto ack = transform::ack( request);

                           auto reply = message::reverse::type( request);
                           reply.buffer = std::move( request.buffer);
                           reply.code = service::code::get( request.service.name);

                           communication::device::blocking::send( request.process.ipc, reply);
                           
                           ack.metric.end = platform::time::clock::type::now();
                           communication::device::blocking::send( communication::instance::outbound::service::manager::device(), ack);
                        }
                     };

                     struct Conversation 
                     {
                        void operator() ( message::conversation::connect::callee::Request& request)
                        {
                           auto ack = transform::ack( request);

                           {
                              auto reply = message::reverse::type( request);
                              reply.recording = request.recording;
                              reply.route = request.recording;
                              //reply.code = service::code::get( request.service.name);

                              auto node = reply.route.next();
                              communication::device::blocking::send( node.address, reply);
                           }

                           {
                              // send conversation send
                              message::conversation::callee::Send message;

                              message.buffer = std::move( request.buffer);
                              message.code = service::code::get( request.service.name);
                           }

                           ack.metric.end = platform::time::clock::type::now();
                           communication::device::blocking::send( communication::instance::outbound::service::manager::device(), ack);
                        }
                     };

                     void advertise()
                     {
                        auto transform_service = []( auto& pair)
                        {
                           message::service::advertise::Service service;
                           service.name = pair.first;
                           service.category = "example";
                           service.transaction = decltype( service.transaction)::none;
                           return service;
                        };

                        message::service::Advertise message{ process::handle()};
                        message.alias = instance::alias();
                        message.services.add = algorithm::transform( code::mapping, transform_service);

                        communication::device::blocking::send( communication::instance::outbound::service::manager::device(), message);
                     }
                  } // service
               } // handle

               void start()
               {
                  // connect to the domain
                  communication::instance::connect();

                  handle::service::advertise();

                  auto& device = communication::ipc::inbound::device();

                  auto handler = message::dispatch::handler( device,
                     message::handle::defaults( device),
                     handle::service::Call{},
                     handle::service::Conversation{},
                     message::handle::Shutdown{});

                  message::dispatch::pump( handler, device);
               }

               void main( int argc, char** argv)
               {
                  start();
               }
               
            } // error
         } // <unnamed>
      } // local
   } // example
} // casual

int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::example::local::error::main( argc, argv);
   });
} // main