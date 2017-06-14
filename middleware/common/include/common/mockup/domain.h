//!
//! casual
//!

#ifndef CASUAL_COMMON_MOCKUP_DOMAIN_H_
#define CASUAL_COMMON_MOCKUP_DOMAIN_H_

#include "common/mockup/ipc.h"
#include "common/mockup/transform.h"

#include "common/message/service.h"
#include "common/message/server.h"
#include "common/message/transaction.h"
#include "common/message/queue.h"
#include "common/message/conversation.h"


#include "common/file.h"
#include "common/domain.h"


#include <vector>

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace domain
         {
            namespace configuration
            {
               struct Domain;
            } // configuration
         } // domain
      } // message

      namespace mockup
      {



         namespace domain
         {

            using dispatch_type = communication::ipc::dispatch::Handler;

            namespace service
            {
               struct Echo
               {
                  void operator()( message::service::call::callee::Request& reqeust);
               };

               namespace conversation
               {

                  dispatch_type echo();

               } // conversation


               struct Manager
               {
                  Manager();
                  Manager( dispatch_type&& handler);

                  ~Manager();

               private:

                  struct State
                  {
                     std::map< std::string, common::message::service::lookup::Reply> services;
                     std::vector< platform::ipc::id::type> traffic_monitors;
                  };


                  dispatch_type default_handler();

                  State m_state;
                  ipc::Replier m_replier;
               };


            } // server


            struct Manager
            {
               Manager();
               Manager( dispatch_type&& handler, const common::domain::Identity& identity = common::domain::Identity{ "unittest-domain"});
               Manager( message::domain::configuration::Domain domain);


               ~Manager();

               process::Handle process() const;


            private:

               struct Implementation;
               common::move::basic_pimpl< Implementation> m_implementation;
            };







            namespace transaction
            {
               struct Manager
               {
                  Manager();
                  Manager( dispatch_type&& handler);

               private:

                  dispatch_type default_handler();

                  ipc::Replier m_replier;
               };

            } // transaction


            namespace queue
            {
               struct Broker
               {
                  Broker();
                  Broker( dispatch_type&& handler);

               private:
                  dispatch_type default_handler();

                  using Message = message::queue::dequeue::Reply::Message;

                  std::unordered_map< std::string, std::queue< Message>> m_queues;

                  ipc::Replier m_replier;
               };


            } // queue


            namespace echo
            {

               namespace create
               {
                  message::service::advertise::Service service( std::string name);
               } // create

               struct Server
               {
                  Server( std::vector< message::service::advertise::Service> services);
                  Server( message::service::advertise::Service service);

                  void advertise( std::vector< message::service::advertise::Service> services) const;
                  void undadvertise( std::vector< std::string> services) const;


                  void send_ack() const;

                  process::Handle process() const;

               private:
                  ipc::Replier m_replier;
               };

            } // echo

            namespace minimal
            {
               //!
               //! Exposes the following services, that just echos the payload
               //!
               //! - service1
               //! - service2
               //! - removed_ipc_queue <- corresponds to an ipc-queue that does not exists
               //!
               struct Domain
               {
                  Domain();

                  domain::Manager domain;
                  service::Manager service;
                  transaction::Manager tm;

                  echo::Server server;
               };


            } // minimal


         } // domain

      } // mockup
   } // common
} // casual

#endif // CASUAL_COMMON_MOCKUP_DOMAIN_H_
