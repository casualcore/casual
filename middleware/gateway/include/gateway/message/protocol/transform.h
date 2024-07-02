//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/service.h"
#include "queue/common/ipc/message.h"

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
         result.service = std::move( message.service);
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
         result.service = std::move( message.service);
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
         result.transaction = std::move( message.transaction);
         return result;
      }

      inline auto from( common::message::service::call::v1_2::Reply&& message)
      {
         common::message::service::call::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.code = message.code;
         result.transaction = std::move( message.transaction);
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
         result.service = std::move( message.service);
         result.trid = std::move( message.trid);
         return result;
      }

      inline auto from( common::message::conversation::connect::v1_2::callee::Request&& message)
      {
         common::message::conversation::connect::callee::Request result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         result.buffer = std::move( message.buffer);
         result.parent.service = std::move( message.parent);
         result.pending = message.pending;
         result.service = std::move( message.service);
         result.trid = std::move( message.trid);
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


      template<>
      inline casual::queue::ipc::message::group::dequeue::v1_2::Reply to( casual::queue::ipc::message::group::dequeue::Reply&& message)
      {
         casual::queue::ipc::message::group::dequeue::v1_2::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;

         if( message.message)
            result.message.push_back( std::move( *message.message));
         return result;
      }

      inline auto from( casual::queue::ipc::message::group::dequeue::v1_2::Reply&& message)
      {
         casual::queue::ipc::message::group::dequeue::Reply result;
         result.correlation = message.correlation;
         result.execution = message.execution;
         if( ! message.message.empty())
            result.message = std::move( message.message.front());
         else
            result.code = decltype( result.code)::no_message;

         return result;
      }

      
   } // gateway::message::protocol::transform
   
} // casual