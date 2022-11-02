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

#include <iostream>

namespace casual
{
   namespace cli::message
   {  
      using dispatch = common::message::dispatch::handle::protocol::basic< common::communication::stream::inbound::Device::complete_type>;

      template< common::message::Type type>
      using base_message = common::message::basic_request< type>;

      namespace pipe
      {
         using base_done = base_message< common::message::Type::cli_pipe_done>;
         struct Done : base_done
         {
            using base_done::base_done;

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_done::serialize( archive);
            )
         };


      } // pipe

      struct Transaction
      {
         common::transaction::ID trid;
         common::code::tx code = common::code::tx::ok;

         inline explicit operator bool () const { return ! common::transaction::id::null( trid);}
         inline bool committable() const { return code == common::code::tx::ok;}

         inline friend bool operator == ( const Transaction& lhs, const Transaction& rhs) { return lhs.trid == rhs.trid;};


         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( trid);
            CASUAL_SERIALIZE( code);
         )
      };

      namespace transaction
      {
         using base_directive = base_message< common::message::Type::cli_transaction_directive>;
         struct Directive : base_directive
         {
            using base_directive::base_directive;

            enum class Context : short
            {
               absent,
               single,
               compound,
            };

            message::Transaction transaction;

            inline auto context() const
            {
               if( ! process)
                  return Context::absent;
               if( transaction.trid)
                  return Context::single;
               return Context::compound;
            }

            inline explicit operator bool() const { return context() != Context::absent;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_directive::serialize( archive);
               CASUAL_SERIALIZE( transaction);
            )
         };

         namespace directive
         {
            inline auto single()
            {
               Directive result{ common::process::handle()};
               result.transaction.trid = common::transaction::id::create();
               return result;
            }

            inline auto compound()
            {
               Directive result{ common::process::handle()};
               return result;
            }

            using Terminated = base_message< common::message::Type::cli_transaction_directive_terminated>;
         } // directive

         using base_associated = base_message< common::message::Type::cli_transaction_associated>;
         struct Associated : base_associated
         {
            using base_associated::base_associated;

            common::transaction::ID trid;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               base_associated::serialize( archive);
               CASUAL_SERIALIZE( trid);
            )
         };

         //! finalizes transactions - commit or rollback...
         using base_finalize = base_message< common::message::Type::cli_transaction_finalize>;
         struct Finalize : base_finalize
         {
            using base_finalize::base_finalize;

            std::vector< message::Transaction> transactions;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               base_finalize::serialize( archive);
               CASUAL_SERIALIZE( transactions);
            )
         };

         //! used to propagate transaction (alone) downstream (mostly when a dequeue gives no-message)
         using base_propagate = base_message< common::message::Type::cli_transaction_propagate>;
         struct Propagate : base_propagate
         {
            using base_propagate::base_propagate;

            message::Transaction transaction;
            
            CASUAL_CONST_CORRECT_SERIALIZE(
               base_propagate::serialize( archive);
               CASUAL_SERIALIZE( transaction);
            )
         };

      } // transaction

      namespace queue
      {
         namespace message
         {
            using id_base = base_message< common::message::Type::cli_queue_message_id>;
            struct ID : id_base
            {
               using id_base::id_base;
               common::Uuid id;
               cli::message::Transaction transaction;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  id_base::serialize( archive);
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( transaction);
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

         using message_base = base_message< common::message::Type::cli_queue_message>;
         struct Message : message_base
         {
            using message_base::message_base;

            common::Uuid id;
            message::Attributes attributes;
            common::buffer::Payload payload;
            cli::message::Transaction transaction;

            CASUAL_CONST_CORRECT_SERIALIZE(
               message_base::serialize( archive);
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( attributes);
               CASUAL_SERIALIZE( payload);
               CASUAL_SERIALIZE( transaction);
            ) 
         };

      } // queue

      namespace payload
      {
         using message_base = base_message< common::message::Type::cli_payload>;
         struct Message : message_base
         {
            using message_base::message_base;

            common::message::service::Code code;
            common::buffer::Payload payload;
            message::Transaction transaction;

            CASUAL_CONST_CORRECT_SERIALIZE(
               message_base::serialize( archive);
               CASUAL_SERIALIZE( code);
               CASUAL_SERIALIZE( payload);
               CASUAL_SERIALIZE( transaction);
            )  
         };

         //! @return a _dispatch handler_ that have handlers for the messages
         //!  `payload::Message`and `queue::Message`, these uses the same 
         //!  provided `handler`. Useful when one wants to handle both messages
         //!  in a generic way. 
         template< typename H>
         auto handler( H handler)
         {
            return dispatch::compose< 
               payload::Message, 
               queue::Message>( std::move( handler));
         }

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
