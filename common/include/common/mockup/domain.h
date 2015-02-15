//!
//! domain.h
//!
//! Created on: Feb 15, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MOCKUP_DOMAIN_H_
#define CASUAL_COMMON_MOCKUP_DOMAIN_H_

#include "common/mockup/ipc.h"
#include "common/mockup/transform.h"

#include "common/message/server.h"
#include "common/message/transaction.h"

#include "common/ipc.h"


#include <vector>

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         namespace broker
         {

            namespace server
            {
               struct Connect
               {
                  using message_type = common::message::server::connect::Request;
                  using reply_type = common::message::server::connect::Reply;

                  std::vector< common::ipc::message::Complete> operator () ( message_type message);
               };
            }

            namespace client
            {
               struct Connect
               {
                  using message_type = common::message::transaction::client::connect::Request;
                  using reply_type = common::message::transaction::client::connect::Reply;

                  std::vector< common::ipc::message::Complete> operator () ( message_type message);
               };
            }

            struct Lookup
            {
               Lookup( std::vector< common::message::service::name::lookup::Reply> replies);

               using message_type = common::message::service::name::lookup::Request;
               using reply_type = common::message::service::name::lookup::Reply;

               std::vector< common::ipc::message::Complete> operator () ( message_type message);

               std::map< std::string, common::message::service::name::lookup::Reply> m_broker;
            };

         } // broker

         namespace service
         {



            struct Call
            {
               using message_type = common::message::service::callee::Call;
               using reply_type = common::message::service::Reply;

               Call( std::vector< std::pair< std::string, reply_type>> replies);

               std::vector< common::ipc::message::Complete> operator () ( message_type message);

               std::map< std::string, reply_type> m_server;
            };

         } // service

         namespace transaction
         {



            struct Begin
            {
               using message_type = common::message::transaction::begin::Request;
               using reply_type = common::message::transaction::begin::Reply;

               std::vector< common::ipc::message::Complete> operator () ( message_type message);
            };

            struct Commit
            {
               using message_type = common::message::transaction::commit::Request;
               using reply_type = common::message::transaction::commit::Reply;

               std::vector< common::ipc::message::Complete> operator () ( message_type message);
            };

            struct Rollback
            {
               using message_type = common::message::transaction::rollback::Request;
               using reply_type = common::message::transaction::rollback::Reply;

               std::vector< common::ipc::message::Complete> operator () ( message_type message);
            };

         } // transaction


         namespace create
         {
            namespace lookup
            {
               common::message::service::name::lookup::Reply reply(
                     const std::string& service,
                     platform::queue_id_type queue,
                     std::chrono::microseconds timeout = std::chrono::microseconds::zero());

            } // lookup

            transform::Handler broker( std::vector< message::service::name::lookup::Reply> replies);
            transform::Handler broker();


            transform::Handler server( std::vector< std::pair< std::string, message::service::Reply>> replies);

            namespace transaction
            {
               transform::Handler manager();
            } // transaction

         } // create

      } // mockup
   } // common
} // casual

#endif // CASUAL_COMMON_MOCKUP_DOMAIN_H_
