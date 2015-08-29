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
#include "common/mockup/reply.h"

#include "common/message/service.h"
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
               Lookup( std::vector< common::message::service::lookup::Reply> replies);

               using message_type = common::message::service::lookup::Request;
               using reply_type = common::message::service::lookup::Reply;

               std::vector< common::ipc::message::Complete> operator () ( message_type message);

               std::map< std::string, common::message::service::lookup::Reply> m_broker;
            };

         } // broker

         namespace service
         {



            struct Call
            {
               using message_type = common::message::service::call::callee::Request;
               using reply_type = common::message::service::call::Reply;

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
               common::message::service::lookup::Reply reply(
                     const std::string& service,
                     platform::queue_id_type queue,
                     std::chrono::microseconds timeout = std::chrono::microseconds::zero());

            } // lookup

            transform::Handler broker( std::vector< message::service::lookup::Reply> replies);
            transform::Handler broker();


            transform::Handler server( std::vector< std::pair< std::string, message::service::call::Reply>> replies);

            namespace transaction
            {
               transform::Handler manager();
            } // transaction

         } // create


         namespace domain
         {
            namespace service
            {
               struct Echo
               {
                  std::vector< reply::result_t> operator()( message::service::call::callee::Request reqeust);
               };
            } // server

            namespace broker
            {
               struct Lookup
               {
                  Lookup( std::vector< common::message::service::lookup::Reply> replies);
                  std::vector< reply::result_t> operator()( message::service::lookup::Request reqeust);
               private:
                  std::map< std::string, common::message::service::lookup::Reply> m_services;
               };


               namespace transaction
               {
                  namespace client
                  {
                     struct Connect
                     {
                        Connect( message::transaction::client::connect::Reply reply) : m_reply{ std::move( reply)} {}

                        std::vector< reply::result_t> operator () ( message::transaction::client::connect::Request r);
                     private:
                        message::transaction::client::connect::Reply m_reply;

                     }; // connect

                  } // client

               } // transaction


            } // broker

            struct Broker
            {
               Broker();

               template< typename... Args>
               Broker( Args&& ...args) : Broker( default_handler( std::forward< Args>( args)...)) {}

               ~Broker();

            private:

               struct State
               {

                  std::map< std::string, common::message::service::lookup::Reply> services;
                  std::map< common::Uuid, common::process::Handle> singeltons;
                  std::map< common::Uuid, std::vector< common::message::lookup::process::Request>> singelton_request;
               };



               Broker( reply::Handler handler);


               reply::Handler default_handler();

               template< typename... Args>
               reply::Handler default_handler( Args&& ...args)
               {
                  auto result = default_handler();
                  result.insert( std::forward< Args>( args)...);
                  return result;
               }

               State m_state;
               ipc::Replier m_replier;
               ipc::Link m_broker_replier_link;
            };

            namespace transaction
            {


               struct Manager
               {
                  Manager();

                  template< typename... Args>
                  Manager( Args&& ...args) : Manager( default_handler( std::forward< Args>( args)...)) {}

               private:

                  reply::Handler default_handler();

                  template< typename... Args>
                  reply::Handler default_handler( Args&& ...args)
                  {
                     auto result = default_handler();
                     result.insert( std::forward< Args>( args)...);
                     return result;
                  }

                  Manager( reply::Handler handler);

                  ipc::Replier m_replier;
                  ipc::Link m_tm_replier_link;
               };

            } // transaction


            struct Domain
            {
               Domain();

               ipc::Replier server1;
               Broker m_broker;
               transaction::Manager m_manager;

            };


         } // domain

      } // mockup
   } // common
} // casual

#endif // CASUAL_COMMON_MOCKUP_DOMAIN_H_
