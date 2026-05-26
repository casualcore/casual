//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/service.h"
#include "common/message/conversation.h"
#include "queue/common/ipc/message.h"
#include "domain/message/discovery.h"

namespace casual
{
   namespace gateway::message::protocol::transform
   {
      template< typename R, typename M>
      R to( M&& message) = delete;

      template<>
      inline common::message::service::call::v1_2::callee::Request to( common::message::service::call::callee::Request&& message)
      {
         common::message::service::call::v1_2::callee::Request result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.flags = message.flags;
         result.parent = std::move( message.parent.service);
         result.pending = message.pending;
         result.service.name = std::move( message.service.name);
         result.service.timeout.duration = message.deadline.remaining.value_or( common::chronology::duration{});
         result.trid = std::move( message.trid);
         return result;
      }

      template<>
      inline common::message::service::call::v1_4::callee::Request to( common::message::service::call::callee::Request&& message)
      {
         common::message::service::call::v1_4::callee::Request result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.service = std::move( message.service);
         result.deadline = std::move( message.deadline);
         result.parent = std::move( message.parent);
         result.trid = std::move( message.trid);
         result.flags = message.flags;
         result.pending = message.pending;
         result.buffer = std::move( message.buffer);

         // header is not used over the wire.
         // result.header = std::move( message.header);

         return result;
      }

      inline auto from( common::message::service::call::v1_4::callee::Request&& message)
      {
         common::message::service::call::callee::Request result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.flags = message.flags;
         result.parent = std::move( message.parent);
         result.pending = message.pending;
         result.service = std::move( message.service);
         result.deadline = std::move( message.deadline);
         result.trid = std::move( message.trid);
         
         return result;
      }


      inline auto from( common::message::service::call::v1_2::callee::Request&& message)
      {
         common::message::service::call::callee::Request result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.flags = message.flags;
         result.parent.service = std::move( message.parent);
         result.pending = message.pending;
         result.service.name = std::move( message.service.name);
         
         if( message.service.timeout.duration > common::chronology::duration{})
            result.deadline.remaining = message.service.timeout.duration;

         result.trid = std::move( message.trid);
         return result;
      }

      template<>
      inline common::message::service::call::v1_2::Reply to( common::message::service::call::Reply&& message)
      {
         common::message::service::call::v1_2::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.code = message.code;
         result.transaction.state = message.transaction_state;
         return result;
      }

      template<>
      inline common::message::service::call::v1_4::Reply to( common::message::service::call::Reply&& message)
      {
         common::message::service::call::v1_4::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.code = message.code;
         result.transaction_state = message.transaction_state;
         return result;
      }

      inline auto from( common::message::service::call::v1_2::Reply&& message)
      {
         common::message::service::call::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.code = message.code;
         result.transaction_state = message.transaction.state;
         return result;
      }

      inline auto from( common::message::service::call::v1_4::Reply&& message)
      {
         common::message::service::call::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.code = message.code;
         result.transaction_state = message.transaction_state;
         return result;
      }

      template<>
      inline common::message::conversation::connect::v1_2::callee::Request to( common::message::conversation::connect::callee::Request&& message)
      {
         common::message::conversation::connect::v1_2::callee::Request result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.parent = std::move( message.parent.service);
         result.pending = message.pending;
         result.duplex = message.duplex;
         result.service.name = std::move( message.service.name);
         result.service.timeout.duration = message.deadline.remaining.value_or( common::chronology::duration{});
         result.trid = std::move( message.trid);
         return result;
      }

      template<>
      inline common::message::conversation::connect::v1_5::callee::Request to( common::message::conversation::connect::callee::Request&& message)
      {
         common::message::conversation::connect::v1_5::callee::Request result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.service = std::move( message.service);
         result.parent = std::move( message.parent);
         result.deadline = std::move( message.deadline);
         result.trid = std::move( message.trid);
         result.pending = message.pending;
         result.duplex = message.duplex;
         result.buffer.data = std::move( message.buffer.data);
         result.buffer.type = std::move( message.buffer.type);
         return result;
      }

      inline auto from( common::message::conversation::connect::v1_2::callee::Request&& message)
      {
         common::message::conversation::connect::callee::Request result{ message.process};
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.parent.service = std::move( message.parent);
         result.pending = message.pending;
         result.duplex = message.duplex;
         result.service.name = std::move( message.service.name);
         
         if( message.service.timeout.duration > common::chronology::duration{})
            result.deadline.remaining = message.service.timeout.duration;
         
         result.trid = std::move( message.trid);
         return result;
      }

      inline auto from( common::message::conversation::connect::v1_5::callee::Request&& message)
      {
         common::message::conversation::connect::callee::Request result{ message.process};
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.service = std::move( message.service);
         result.parent = std::move( message.parent);
         result.deadline = std::move( message.deadline);
         result.trid = std::move( message.trid);
         result.pending = message.pending;
         result.duplex = message.duplex;
         result.buffer.data = std::move( message.buffer.data);
         result.buffer.type = std::move( message.buffer.type);
         
         return result;
      }

      template<>
      inline common::message::conversation::v1_5::callee::Send to( common::message::conversation::callee::Send&& message)
      {
         common::message::conversation::v1_5::callee::Send result;
         result.correlation = message.correlation;
         result.execution = message.execution;         
         result.transaction_state = message.transaction_state;
         result.buffer.data = std::move( message.buffer.data);
         result.buffer.type = std::move( message.buffer.type);

         using duplex_t = decltype( message.duplex);
         using duplex_v1_5_t = decltype( result.duplex);
         
         if( message.duplex == duplex_t::terminated)
         {
            // duplex is not of interest.
            result.duplex = {};

            // we set code.result to NOT "absent" to signal the receiver that this is a "terminated" message, 
            // and that it should not expect any more messages in this conversation. 
            // This is needed since v1.5 protocol does not have the "terminated" duplex type.
            result.code = message.code;
            if( result.code.result == common::code::xatmi::absent)
               result.code.result = common::code::xatmi::ok;
         }
         else
         {
            if( message.duplex == duplex_t::send)
               result.duplex = duplex_v1_5_t::send;
            else if( message.duplex == duplex_t::receive)
               result.duplex = duplex_v1_5_t::receive;

            // this is not a "terminated" message, make sure code.result is "absent", as this is 
            // used to signal NOT "terminated" messages in v1.5 protocol.
            result.code.result = common::code::xatmi::absent;
            result.code.user = message.code.user;

         }

         return result;
      }

      inline auto from( common::message::conversation::v1_5::callee::Send&& message)
      {
         common::message::conversation::callee::Send result;
         result.correlation = message.correlation;
         result.execution = message.execution;

         result.transaction_state = message.transaction_state;
         result.code = message.code;
         result.buffer.data = std::move( message.buffer.data);
         result.buffer.type = std::move( message.buffer.type);

         if( result.code.result != common::code::xatmi::absent)
         {
            // this is a "terminated" message, set duplex accordingly.
            result.duplex = common::message::conversation::duplex::send::Type::terminated;
         }
         else 
         {
            if( message.duplex == decltype( message.duplex)::send)
               result.duplex = decltype( result.duplex)::send;
            else if( message.duplex == decltype( message.duplex)::receive)
               result.duplex = decltype( result.duplex)::receive;
         }

         return result;
      }

      template<>
      inline casual::queue::ipc::message::group::enqueue::v1_5::Request to( casual::queue::ipc::message::group::enqueue::Request&& message)
      {
         casual::queue::ipc::message::group::enqueue::v1_5::Request result{ message.process};
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.name = std::move( message.name);
         result.trid = std::move( message.trid);
         result.message.attributes = casual::queue::ipc::message::group::enqueue::v1_5::Attributes{
            .properties = std::move( message.message.attributes.properties),
            .reply = std::move( message.message.attributes.reply),
            .available = message.message.attributes.available
         };
         result.message.payload = std::move( message.message.payload);
         result.message.id = message.message.id;
         return result;
      }

      inline auto from( casual::queue::ipc::message::group::enqueue::v1_5::Request&& message)
      {
         casual::queue::ipc::message::group::enqueue::Request result{ message.process};
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.name = std::move( message.name);
         result.trid = std::move( message.trid);
         result.message.attributes = casual::queue::ipc::message::Attributes{
            .properties =  std::move( message.message.attributes.properties),
            .reply = std::move( message.message.attributes.reply),
            .available = message.message.attributes.available
         };
         result.message.payload = std::move( message.message.payload);
         result.message.id = message.message.id;
         return result;
      }

      template<>
      inline casual::queue::ipc::message::group::enqueue::v1_2::Reply to( casual::queue::ipc::message::group::enqueue::Reply&& message)
      {
         casual::queue::ipc::message::group::enqueue::v1_2::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.id = message.id;
         return result;
      }

      inline auto from( casual::queue::ipc::message::group::enqueue::v1_2::Reply&& message)
      {
         casual::queue::ipc::message::group::enqueue::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.id = message.id;
         result.code = message.id ? decltype( result.code)::ok : decltype( result.code)::no_queue;
         return result;
      }

      namespace detail
      {
         inline auto to_dequeue_1_5_message( casual::queue::ipc::message::group::dequeue::Message message)
         {
            casual::queue::ipc::message::group::dequeue::v1_5::Message result;
            result.id = message.id;
            result.attributes = std::move( message.attributes);
            result.payload.type = std::move( message.payload.type);
            result.payload.data = std::move( message.payload.data);
            // no header in over-the-wire protocol v1.5. serialization specialization for 1.5/1.2 dequeue makes sure
            // to not serialize the header
            result.redelivered = message.redelivered;
            result.timestamp = message.timestamp;
            return result;
         }
      } // detail

      template<>
      inline casual::queue::ipc::message::group::dequeue::v1_5::Reply to( casual::queue::ipc::message::group::dequeue::Reply&& message)
      {
         casual::queue::ipc::message::group::dequeue::v1_5::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;

         if( message.message)
            result.message = detail::to_dequeue_1_5_message( std::move( *message.message));
         return result;
      }

      template<>
      inline casual::queue::ipc::message::group::dequeue::v1_2::Reply to( casual::queue::ipc::message::group::dequeue::Reply&& message)
      {
         casual::queue::ipc::message::group::dequeue::v1_2::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;

         if( message.message)
            result.message.push_back( detail::to_dequeue_1_5_message( std::move( *message.message)));
         return result;
      }

      namespace detail
      {
         inline auto to_dequeue_message( auto&& message)
         {
            casual::queue::ipc::message::group::dequeue::Message result;
            result.id = message.id;
            result.attributes = casual::queue::ipc::message::Attributes{
               .properties = std::move( message.attributes.properties),
               .reply = std::move( message.attributes.reply),
               .available = message.attributes.available
            };
            result.payload = std::move( message.payload);
            result.redelivered = message.redelivered;
            result.timestamp = message.timestamp;
            return result;
         }
      } // detail

      inline auto from( casual::queue::ipc::message::group::dequeue::v1_5::Reply&& message)
      {
         casual::queue::ipc::message::group::dequeue::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.code = message.code;

         if( message.message)
            result.message = detail::to_dequeue_message( std::move( *message.message));
         
         return result;
      }

      inline auto from( casual::queue::ipc::message::group::dequeue::v1_2::Reply&& message)
      {
         casual::queue::ipc::message::group::dequeue::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         if( ! message.message.empty())
            result.message = detail::to_dequeue_message( std::move( message.message.front()));
         else
            result.code = decltype( result.code)::no_message;

         return result;
      }


      inline auto from( casual::domain::message::discovery::v1_3::Reply&& message)
      {
         casual::domain::message::discovery::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.domain = std::move( message.domain);
         result.content.services = std::move( message.content.services);
         result.content.queues = common::algorithm::transform( message.content.queues, []( auto& queue)
         {
            casual::domain::message::discovery::reply::content::Queue result;
            result.name = std::move( queue.name);
            result.retry.count = queue.retries;
            return result;
         });

         return result;
      }

      template<>
      inline casual::domain::message::discovery::v1_3::Reply to( casual::domain::message::discovery::Reply&& message)
      {
         casual::domain::message::discovery::v1_3::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.domain = std::move( message.domain);
         result.content.services = std::move( message.content.services);
         result.content.queues = common::algorithm::transform( message.content.queues, []( auto& queue)
         {
            casual::domain::message::discovery::reply::content::v1_3::Queue result;
            result.name = std::move( queue.name);
            result.retries = queue.retry.count;
            return result;
         });

         return result;
      }

      
   } // gateway::message::protocol::transform
   
} // casual