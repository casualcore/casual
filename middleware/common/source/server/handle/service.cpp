//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/server/handle/service.h"

#include "common/server/context.h"
#include "common/service/conversation/context.h"
#include "common/buffer/pool.h"
#include "common/transaction/context.h"
#include "common/algorithm/compare.h"
#include "common/communication/ipc.h"

#include "casual/assert.h"

namespace casual
{
   namespace common::server::handle::service
   {
      namespace transform
      {
         namespace local
         {
            namespace
            {

               using Flag = common::service::invoke::Parameter::Flag;

               template< typename M>
               auto parameter( M& message)
               {
                  common::service::invoke::Parameter result{ std::move( message.buffer)};
                  result.service.name = message.service.name;
                  result.parent = std::move( message.parent);

                  if( transaction::context().current())
                     result.flags = Flag::in_transaction;

                  return result;
               }

            } // <unnamed>
         } // local
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
            
            return result;
         }


         common::service::invoke::Parameter parameter( message::service::call::callee::Request& message)
         {
            
            auto result = local::parameter( message);

            using Flag = decltype( message.flags.type());

            if( message.flags.exist( Flag::no_reply))
               result.flags |= decltype( result.flags.type())::no_reply;

            return result;
         }

         common::service::invoke::Parameter parameter( message::conversation::connect::callee::Request& message)
         {
            auto result = local::parameter( message);

            // set flags
            {
               result.flags |= local::Flag::conversation;

               using Duplex = decltype( message.duplex);
               casual::assertion( algorithm::compare::any( message.duplex, Duplex::send, Duplex::receive), "unexpected duplex: ", message);

               result.flags |= message.duplex == Duplex::receive ? local::Flag::receive_only : local::Flag::send_only;
            }


            // reserve descriptor, can "never" fail
            result.descriptor = common::service::conversation::Context::instance().descriptors().reserve( 
               message.correlation,
               message.process,
               message.duplex,
               false  // not the initiator
            );

            // send reply
            {
               auto reply = message::reverse::type( message, process::handle());
               communication::device::blocking::send( message.process.ipc, reply);
            }

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

            reply.code.user = result.code;
            reply.buffer = std::move( result.payload);

            if( result.transaction == common::service::invoke::Result::Transaction::commit)
            {
               reply.code.result = code::xatmi::ok;  
            }
            else
            {
               reply.code.result = code::xatmi::service_fail;
            }

            

            log::line( log::debug, "reply: ", reply);
         }

      } // complement

   } // common::server::handle::service
} // casual
