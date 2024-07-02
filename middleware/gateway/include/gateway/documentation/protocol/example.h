//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/message.h"

namespace casual
{
   namespace gateway
   {
      namespace documentation
      {
         namespace protocol
         {
            namespace example
            {
               namespace detail
               {
                  void fill( gateway::message::domain::connect::Request& message);
                  void fill( gateway::message::domain::connect::Reply& message);

                  void fill( gateway::message::domain::disconnect::Request& message);
                  void fill( gateway::message::domain::disconnect::Reply& message);

                  void fill( casual::domain::message::discovery::Request& message);
                  void fill( casual::domain::message::discovery::Reply& message);
                  void fill( casual::domain::message::discovery::topology::implicit::Update& message);

                  void fill( common::message::service::call::callee::Request& message);
                  void fill( common::message::service::call::v1_2::callee::Request& message);
                  void fill( common::message::service::call::Reply& message);
                  void fill( common::message::service::call::v1_2::Reply& message);

                  void fill( common::message::conversation::connect::callee::Request& message);
                  void fill( common::message::conversation::connect::v1_2::callee::Request& message);
                  void fill( common::message::conversation::connect::Reply& message);

                  void fill( common::message::conversation::callee::Send& message);
                  void fill( common::message::conversation::Disconnect& message);

                  void fill( casual::queue::ipc::message::group::enqueue::Request& message);
                  void fill( casual::queue::ipc::message::group::enqueue::Reply& message);
                  void fill( casual::queue::ipc::message::group::enqueue::v1_2::Reply& message);

                  void fill( casual::queue::ipc::message::group::dequeue::Request& message);
                  void fill( casual::queue::ipc::message::group::dequeue::Reply& message);
                  void fill( casual::queue::ipc::message::group::dequeue::v1_2::Reply& message);

                  void fill( common::message::transaction::resource::prepare::Request& message);
                  void fill( common::message::transaction::resource::prepare::Reply& message);

                  void fill( common::message::transaction::resource::commit::Request& message);
                  void fill( common::message::transaction::resource::commit::Reply& message);

                  void fill( common::message::transaction::resource::rollback::Request& message);
                  void fill( common::message::transaction::resource::rollback::Reply& message);

               } // detail
               
               template< typename M>
               M message() 
               {
                  M result;
                  detail::fill( result);
                  return result;
               }
            } // example

         } // protocol
      } // documentation
   } // gateway
} // casual