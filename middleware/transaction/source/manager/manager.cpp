//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/common.h"

#include "common/environment.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/log.h"
#include "common/exception/casual.h"
#include "common/exception/handle.h"
#include "common/chronology.h"

#include "common/communication/instance.h"

#include "configuration/domain.h"
#include "configuration/file.h"

#include <tx.h>

namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace environment
      {
         namespace log
         {
            std::string file()
            {
               return configuration::directory::domain() + "/transaction/log.db";
            }
         } // log

      } // environment

      Settings::Settings() :
         log{ environment::log::file()}
      {

      }

      Manager::Manager( Settings settings) :
          m_state( common::environment::string( std::move( settings.log)))
      {
         auto start = common::platform::time::clock::type::now();

         log::line( log, "transaction manager start");


         //
         // Set the process variables so children can communicate with us.
         //
         common::environment::variable::process::set(
               common::environment::variable::name::ipc::transaction::manager(),
               common::process::handle());

         //
         // get configuration from domain manager
         //
         action::configure( m_state);


         //
         // Start resource-proxies
         //
         {
            Trace trace{ "start rm-proxy-servers"};

            common::algorithm::for_each(
               m_state.resources,
               action::resource::Instances( m_state));

            //
            // Make sure we wait for the resources to get ready
            //
            auto handler = ipc::device().handler(
               common::message::handle::Shutdown{},
               handle::process::Exit{ m_state},
               handle::resource::reply::Connect{ m_state});



            while( ! m_state.booted())
            {
               handler( ipc::device().blocking_next( handler.types()));
            }

         }


         auto instances = common::algorithm::accumulate(
               m_state.resources, 0,
               []( std::size_t count, const state::resource::Proxy& p) {
                  return count + p.instances.size();
               });

         auto end = common::platform::time::clock::type::now();


         log::line( log::category::information, "transaction manager is on-line - ", 
            m_state.resources.size(), " resources - ", 
            instances, " instances - boot time: ", 
            chronology::duration( end - start));

      }

      Manager::~Manager()
      {
         common::Trace trace{ "transaction::Manager::~Manager"};

         try
         {

            for( auto& resource : m_state.resources)
            {
               resource.concurency = 0;
            }

            algorithm::for_each( m_state.resources, action::resource::Instances{ m_state});

            auto processes = m_state.processes();

            process::lifetime::wait( processes, std::chrono::milliseconds( processes.size() * 100));

         }
         catch( ...)
         {
            common::exception::handle();
         }
      }

      namespace local
      {
         namespace
         {
            namespace message
            {
               void pump( State& state)
               {
                  try
                  {
                     log::line( log, "prepare message dispatch handlers");

                     // prepare message dispatch handlers...
                     auto handler = handle::handlers( state);

                     log::line( log, "start message pump");

                     // Connect to domain
                     communication::instance::connect( communication::instance::identity::transaction::manager);


                     persistent::Writer persist( state.persistent_log);

                     while( true)
                     {
                        Trace trace{ "transaction::Manager message pump"};

                        {
                           persist.begin();

                           if( ! state.outstanding())
                           {
                              // We can only block if our backlog is empty

                              // Removed transaction-timeout from TM, since the semantics are not clear
                              // see commit 559916d9b84e4f84717cead8f2ee7e3d9fd561cd for previous implementation.
                              handler( ipc::device().blocking_next());
                           }

                           // Consume until the queue is empty or we've got pending replies equal to batch::transaction
                           while( handler( ipc::device().non_blocking_next()) &&
                                 state.persistent.replies.size() < common::platform::batch::transaction::persistence)
                           {
                              ; // no-op
                           }
                        }

                        // Check if we have any persistent stuff to handle, if not we don't do persistent commit
                        if( ! state.persistent.replies.empty() || ! state.persistent.requests.empty())
                        {
                           persist.commit();

                           // Send persistent replies to clients
                           {

                              log::line( log, "manager persistent replies: ", state.persistent.replies.size());;

                              auto not_done = common::algorithm::partition(
                                    state.persistent.replies,
                                    common::predicate::negate( action::persistent::Send{ state}));

                              common::algorithm::trim( state.persistent.replies, std::get< 0>( not_done));

                              log::line( log, "manager persistent replies: ", state.persistent.replies.size());
                           }

                           // Send persistent resource requests
                           {
                              log::line( log, "manager persistent request: ", state.persistent.requests.size());

                              auto not_done = common::algorithm::partition(
                                    state.persistent.requests,
                                    common::predicate::negate( action::persistent::Send{ state}));

                              // Move the ones that did not find an idle resource to pending requests
                              common::algorithm::move( std::get< 0>( not_done), state.pending.requests);

                              state.persistent.requests.clear();

                           }
                        }
                        log::line( log, "manager transactions: ", state.transactions.size());
                     }
                  }
                  catch( const exception::casual::Shutdown&)
                  {
                     // We do nothing
                  }
               }

            } // message
         } // <unnamed>
      } // local

      void Manager::start()
      {
         try
         {
            //
            // We're ready to start....
            //
            local::message::pump( m_state);

         }
         catch( const common::exception::signal::Terminate&)
         {
            // we do nothing, and let the dtor take care of business
         }
         catch( ...)
         {
            common::exception::handle();
         }
      }


      const State& Manager::state() const
      {
         return m_state;
      }

   } // transaction
} // casual


