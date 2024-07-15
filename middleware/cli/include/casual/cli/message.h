//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"
#include "common/transaction/id.h"
#include "common/code/tx.h"
#include "common/buffer/type.h"
#include "common/communication/stream.h"
#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"
#include "common/message/service.h"
#include "common/chronology.h"

#include <iostream>

namespace casual
{
   namespace cli::message
   {  
      using dispatch = common::message::dispatch::handle::protocol::basic< common::communication::stream::inbound::Device::complete_type>;

      namespace pipe
      {
         enum class State : int
         {
            ok = 0,
            error, // rollback 
         };

         std::string_view description( State value) noexcept;

         using base_done = common::message::basic_message< common::message::Type::cli_pipe_done>;
         struct Done : base_done
         {
            using base_done::base_done;

            State state = State::ok;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_done::serialize( archive);
               CASUAL_SERIALIZE( state);
            )
         };

      } // pipe

      namespace transaction
      {
         //! used to propagate current transaction downstream.
         using base_current = common::message::basic_message< common::message::Type::cli_transaction_current>;
         struct Current : base_current
         {
            using base_current::base_current;

            common::transaction::ID trid;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               base_current::serialize( archive);
               CASUAL_SERIALIZE( trid);
            )
         };

      } // transaction

      namespace queue
      {
         namespace message
         {
            using id_base = common::message::basic_message< common::message::Type::cli_queue_message_id>;
            struct ID : id_base
            {
               using id_base::id_base;
               common::Uuid id;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  id_base::serialize( archive);
                  CASUAL_SERIALIZE( id);
               )  
            };

            struct Attributes
            {
               std::string properties;
               std::string reply;
               platform::time::point::type available;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( properties);
                  CASUAL_SERIALIZE( reply);
                  CASUAL_SERIALIZE( available);
               ) 
            };

         } // message

         using message_base = common::message::basic_message< common::message::Type::cli_queue_message>;
         struct Message : message_base
         {
            using message_base::message_base;

            common::Uuid id;
            message::Attributes attributes;
            common::buffer::Payload payload;

            CASUAL_CONST_CORRECT_SERIALIZE(
               message_base::serialize( archive);
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( attributes);
               CASUAL_SERIALIZE( payload);
            ) 
         };

      } // queue

      namespace payload
      {
         using message_base = common::message::basic_message< common::message::Type::cli_payload>;
         struct Message : message_base
         {
            using message_base::message_base;

            common::message::service::Code code;
            common::buffer::Payload payload;

            CASUAL_CONST_CORRECT_SERIALIZE(
               message_base::serialize( archive);
               CASUAL_SERIALIZE( code);
               CASUAL_SERIALIZE( payload);
            )  
         };

      } // payload

      namespace to
      {
         template< typename M> 
         struct human
         {
            static void stream( const M& message)
            {
               common::stream::write( std::cout, message, '\n');
            }
         };

         template<> 
         struct human< queue::message::ID>
         {
            static void stream( const queue::message::ID& message);
         };

      } // to
   } // cli::message
} // casual
