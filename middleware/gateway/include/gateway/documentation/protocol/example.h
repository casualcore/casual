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

                  void fill( common::message::gateway::domain::discover::Request& message);
                  void fill( common::message::gateway::domain::discover::Reply& message);

                  void fill( common::message::service::call::callee::Request& message);
                  void fill( common::message::service::call::Reply& message);

                  void fill( common::message::conversation::connect::callee::Request& message);
                  void fill( common::message::conversation::connect::Reply& message);

                  void fill( common::message::conversation::callee::Send& message);
                  void fill( common::message::conversation::Disconnect& message);

                  void fill( common::message::queue::enqueue::Request& message);
                  void fill( common::message::queue::enqueue::Reply& message);

                  void fill( common::message::queue::dequeue::Request& message);
                  void fill( common::message::queue::dequeue::Reply& message);

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