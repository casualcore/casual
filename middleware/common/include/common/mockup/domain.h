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


#include <vector>

namespace casual
{
   namespace common
   {
      namespace mockup
      {



         namespace domain
         {

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
               Manager( message::dispatch::Handler&& handler);

               template< typename... Args>
               Manager( Args&& ...args) : Manager( default_handler( std::forward< Args>( args)...)) {}

               ~Manager();

               inline process::Handle process() const { return m_replier.process();}

            private:
               struct State
               {

                  std::map< common::Uuid, common::process::Handle> singeltons;
                  std::vector< common::message::domain::process::lookup::Request> pending;
                  std::vector< common::process::Handle> executables;
               };

               message::dispatch::Handler default_handler();

               template< typename... Args>
               message::dispatch::Handler default_handler( Args&& ...args)
               {
                  auto result = default_handler();
                  result.insert( std::forward< Args>( args)...);
                  return result;
               }

               State m_state;
               ipc::Replier m_replier;
            };





            struct Broker
            {
               Broker();

               template< typename... Args>
               Broker( Args&& ...args) : Broker( default_handler( std::forward< Args>( args)...)) {}

               Broker( message::dispatch::Handler&& handler);

               ~Broker();

            private:

               struct State
               {
                  std::map< std::string, common::message::service::lookup::Reply> services;
                  std::vector< platform::ipc::id::type> traffic_monitors;
               };


               message::dispatch::Handler default_handler();

               template< typename... Args>
               message::dispatch::Handler default_handler( Args&& ...args)
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

                  message::dispatch::Handler default_handler();

                  template< typename... Args>
                  message::dispatch::Handler default_handler( Args&& ...args)
                  {
                     auto result = default_handler();
                     result.insert( std::forward< Args>( args)...);
                     return result;
                  }

                  Manager( message::dispatch::Handler handler);

                  ipc::Replier m_replier;
               };

            } // transaction

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
