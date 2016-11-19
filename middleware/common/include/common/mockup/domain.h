//!
//! casual
//!

#ifndef CASUAL_COMMON_MOCKUP_DOMAIN_H_
#define CASUAL_COMMON_MOCKUP_DOMAIN_H_

#include "common/mockup/ipc.h"
#include "common/mockup/transform.h"

#include "common/message/domain.h"
#include "common/message/service.h"
#include "common/message/server.h"
#include "common/message/transaction.h"
#include "common/message/queue.h"

#include "common/file.h"
#include "common/domain.h"


#include <vector>

namespace casual
{
   namespace common
   {
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
            } // server


            struct Manager
            {
               Manager();
               Manager( dispatch_type&& handler, const common::domain::Identity& identity = common::domain::Identity{ "unittest-domain"});

               ~Manager();

               inline process::Handle process() const { return m_replier.process();}

            private:
               struct State
               {

                  std::map< common::Uuid, common::process::Handle> singeltons;
                  std::vector< common::message::domain::process::lookup::Request> pending;
                  std::vector< common::process::Handle> executables;
                  std::vector< common::process::Handle> event_listeners;
               };

               dispatch_type default_handler();

               template< typename... Args>
               dispatch_type default_handler( Args&& ...args)
               {
                  auto result = default_handler();
                  result.insert( std::forward< Args>( args)...);
                  return result;
               }

               State m_state;
               ipc::Replier m_replier;
               common::file::scoped::Path m_singlton;
            };





            struct Broker
            {
               Broker();

               template< typename... Args>
               Broker( Args&& ...args) : Broker( default_handler( std::forward< Args>( args)...)) {}

               Broker( dispatch_type&& handler);

               ~Broker();

            private:

               struct State
               {
                  std::map< std::string, common::message::service::lookup::Reply> services;
                  std::vector< platform::ipc::id::type> traffic_monitors;
               };


               dispatch_type default_handler();

               template< typename... Args>
               dispatch_type default_handler( Args&& ...args)
               {
                  auto result = default_handler();
                  result.insert( std::forward< Args>( args)...);
                  return result;
               }

               State m_state;
               ipc::Replier m_replier;
            };

            namespace transaction
            {


               struct Manager
               {
                  Manager();

                  template< typename... Args>
                  Manager( Args&& ...args) : Manager( default_handler( std::forward< Args>( args)...)) {}

               private:

                  dispatch_type default_handler();

                  template< typename... Args>
                  dispatch_type default_handler( Args&& ...args)
                  {
                     auto result = default_handler();
                     result.insert( std::forward< Args>( args)...);
                     return result;
                  }

                  Manager( dispatch_type handler);

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
                  message::service::advertise::Service service(
                        std::string name,
                        std::chrono::microseconds timeout = std::chrono::microseconds::zero());
               } // create

               struct Server
               {
                  Server( std::vector< message::service::advertise::Service> services);
                  Server( message::service::advertise::Service service);

                  void advertise( std::vector< message::service::advertise::Service> services) const;
                  void undadvertise( std::vector< std::string> services) const;


                  void send_ack( std::string service) const;

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
               //! - service3_2ms_timout
               //! - removed_ipc_queue <- corresponds to an ipc-queue that does not exists
               //!
               struct Domain
               {
                  Domain();

                  domain::Manager domain;
                  Broker broker;
                  transaction::Manager tm;

                  echo::Server server;
               };


            } // minimal


         } // domain

      } // mockup
   } // common
} // casual

#endif // CASUAL_COMMON_MOCKUP_DOMAIN_H_
