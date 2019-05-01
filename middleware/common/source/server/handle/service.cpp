//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/server/handle/service.h"

#include "common/server/context.h"
#include "common/service/conversation/context.h"
#include "common/buffer/pool.h"

namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace handle
         {
            namespace service
            {
               namespace transform
               {
                  message::service::call::Reply reply( const message::service::call::callee::Request& message)
                  {
                     message::service::call::Reply result;

                     result.correlation = message.correlation;
                     result.buffer = buffer::Payload{ nullptr};
                     result.code.result = code::xatmi::service_error;

                     return result;
                  }

                  message::conversation::callee::Send reply( const message::conversation::connect::callee::Request& message)
                  {
                     message::conversation::callee::Send result;

                     result.correlation = message.correlation;
                     result.buffer = buffer::Payload{ nullptr};
                     result.code.result = code::xatmi::service_error;
                     result.route = message.recording;

                     return result;
                  }


                  common::service::invoke::Parameter parameter( message::service::call::callee::Request& message)
                  {
                     common::service::invoke::Parameter result{ std::move( message.buffer)};

                     constexpr auto valid_flags = ~common::service::invoke::Parameter::Flags{};

                     result.service.name = message.service.name;
                     result.parent = std::move( message.parent);
                     result.flags = valid_flags.convert( message.flags);

                     return result;
                  }

                  common::service::invoke::Parameter parameter( message::conversation::connect::callee::Request& message)
                  {
                     common::service::invoke::Parameter result{ std::move( message.buffer)};

                     constexpr auto valid_flags = ~common::service::invoke::Parameter::Flags{};

                     result.service.name = std::move( message.service.name);
                     result.parent = std::move( message.parent);
                     result.flags = valid_flags.convert( message.flags);

                     auto& descriptor = common::service::conversation::Context::instance().descriptors().reserve( message.correlation);
                     result.descriptor = descriptor.descriptor;

                     return result;

                  }

               } // transform

               namespace complement
               {
                  void reply( common::service::invoke::Result&& result, message::service::call::Reply& reply)
                  {
                     Trace trace{ "server::handle::service::complement::reply"};

                     log::line( log::debug, "result: ", result);

                     reply.code.user = result.code;
                     reply.buffer = std::move( result.payload);

                     if( result.transaction == common::service::invoke::Result::Transaction::commit)
                     {
                        reply.transaction.state = message::service::Transaction::State::active;
                        reply.code.result = code::xatmi::ok;
                     }
                     else
                     {
                        reply.transaction.state = message::service::Transaction::State::rollback;
                        reply.code.result = code::xatmi::service_fail;
                     }

                     log::line( log::debug, "reply: ", reply);
                  }


                  void reply( common::service::invoke::Result&& result, message::conversation::callee::Send& reply)
                  {
                     Trace trace{ "server::handle::service::complement::reply"};

                     log::line( log::debug, "result: ", result);

                     if( result.transaction == common::service::invoke::Result::Transaction::commit)
                     {
                        reply.events = common::service::conversation::Event::service_success;
                        reply.code.result = code::xatmi::ok;
                     }
                     else
                     {
                        reply.events = common::service::conversation::Event::service_fail;
                        reply.code.result = code::xatmi::service_fail;
                     }

                     reply.buffer = std::move( result.payload);

                     log::line( log::debug, "reply: ", reply);
                  }

               } // complement
            }
         }

      } // server
   } // common

} // casual
