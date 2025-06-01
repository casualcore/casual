//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "server/handle/call.h"

#include "server/context.h"

#include "service/conversation/context.h"

#include "transaction/context.h"

#include "common/buffer/pool.h"
#include "common/algorithm/compare.h"
#include "common/communication/ipc.h"

#include "casual/assert.h"

namespace casual
{
   namespace server::handle::detail
   {
      namespace transform
      {
         namespace local
         {
            namespace
            {

               using Flag = service::invoke::Parameter::Flag;

               template< typename M>
               auto parameter( M& message)
               {
                  service::invoke::Parameter result{
                     .service = { .name = message.service.name},
                     .header = std::move( message.header),
                     .parent = message.parent,
                     .payload = std::move( message.buffer)
                  };
                  
                  if( transaction::context().current())
                     result.flags = Flag::in_transaction;

                  return result;
               }

            } // <unnamed>
         } // local
         common::message::service::call::Reply reply( const common::message::service::call::callee::Request& message)
         {
            common::message::service::call::Reply result;

            result.correlation = message.correlation;
            result.buffer = common::buffer::Payload{ nullptr};
            result.code.result = common::code::xatmi::service_error;

            return result;
         }

         common::message::conversation::callee::Send reply( const common::message::conversation::connect::callee::Request& message)
         {
            common::message::conversation::callee::Send result;

            result.correlation = message.correlation;
            result.buffer = common::buffer::Payload{ nullptr};
            result.code.result = common::code::xatmi::service_error;

            return result;
         }


         service::invoke::Parameter parameter( common::message::service::call::callee::Request& message)
         {
            
            auto result = local::parameter( message);

            using Flag = decltype( message.flags);

            if( common::flag::contains( message.flags, Flag::no_reply))
               result.flags |= decltype( result.flags)::no_reply;

            return result;
         }

         service::invoke::Parameter parameter( common::message::conversation::connect::callee::Request& message)
         {
            auto result = local::parameter( message);

            // set flags
            {
               result.flags |= local::Flag::conversation;

               using Duplex = decltype( message.duplex);
               casual::assertion( common::algorithm::compare::any( message.duplex, Duplex::send, Duplex::receive), "unexpected duplex: ", message);

               result.flags |= message.duplex == Duplex::receive ? local::Flag::receive_only : local::Flag::send_only;
            }


            // reserve descriptor, can "never" fail
            result.descriptor = casual::service::conversation::context().descriptors().reserve( 
               message.correlation,
               message.process,
               message.duplex,
               false  // not the initiator
            );

            // send reply
            {
               auto reply = common::message::reverse::type( message, common::process::handle());
               common::communication::device::blocking::send( message.process.ipc, reply);
            }

            return result;
         }

      } // transform

      namespace complement
      {
         void reply( service::invoke::Result&& result, common::message::service::call::Reply& reply)
         {
            common::Trace trace{ "server::handle::service::complement::reply"};
            common::log::debug( "result: ", result);

            reply.code.user = result.code.user;
            reply.buffer = std::move( result.payload);

            if( result.code.result == common::flag::xatmi::Return::success)
            {
               reply.transaction_state = decltype( reply.transaction_state)::ok;
               reply.code.result = common::code::xatmi::ok;
            }
            else
            {
               reply.transaction_state = decltype( reply.transaction_state)::rollback;
               reply.code.result = common::code::xatmi::service_fail;
            }

            common::log::debug( "reply: ", reply);
         }


         void reply( service::invoke::Result&& result, common::message::conversation::callee::Send& reply)
         {
            common::Trace trace{ "server::handle::service::complement::reply"};
            common::log::debug( "result: ", result);

            reply.code.user = result.code.user;
            reply.buffer = std::move( result.payload);

            // we terminate the conversation -> we're doing a service return.
            reply.duplex = decltype( reply.duplex)::terminated;

            if( result.code.result == common::flag::xatmi::Return::success)
               reply.code.result = common::code::xatmi::ok;  
            else
               reply.code.result = common::code::xatmi::service_fail;


            common::log::debug( "reply: ", reply);
         }

      } // complement

   } // server::handle::detail
} // casual
