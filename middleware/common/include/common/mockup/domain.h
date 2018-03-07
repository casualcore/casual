//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_MOCKUP_DOMAIN_H_
#define CASUAL_COMMON_MOCKUP_DOMAIN_H_

#include "common/mockup/ipc.h"
#include "common/mockup/transform.h"

#include "common/message/service.h"
#include "common/message/server.h"
#include "common/message/conversation.h"


#include "common/file.h"
#include "common/domain.h"

#include "common/pimpl.h"


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
               struct Manager
               {
                  Manager();
                  Manager( dispatch_type&& handler);

                  ~Manager();

               private:
                  struct Implementation;
                  common::move::basic_pimpl< Implementation> m_implementation;
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

                  ~Manager();

               private:
                  struct Implementation;
                  common::move::basic_pimpl< Implementation> m_implementation;
               };

            } // transaction


            namespace queue
            {
               struct Manager
               {
                  Manager();
                  Manager( dispatch_type&& handler);
                  ~Manager();

               private:
                  struct Implementation;
                  common::move::basic_pimpl< Implementation> m_implementation;
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
