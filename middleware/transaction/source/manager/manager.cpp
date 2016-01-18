//!
//! monitor.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!

#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/manager/admin/server.h"


#include "common/server/handle.h"
#include "common/server/lifetime.h"
#include "common/trace.h"
#include "common/environment.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/log.h"


#include "config/domain.h"
#include "config/file.h"


#include <tx.h>

using namespace casual::common;


extern "C"
{
   extern void casual_listTransactions( TPSVCINFO *serviceInfo);
}


namespace casual
{
   namespace transaction
   {
      namespace environment
      {
         namespace log
         {
            std::string file()
            {
               return config::file::domain() + "/transaction/log.db";
            }
         } // log

      } // environment

      Settings::Settings() :
         log{ environment::log::file()}, configuration{ common::environment::file::installedConfiguration()}
      {

      }

      Manager::Manager( const Settings& settings) :
          m_state( settings.log)
      {
         auto start = common::platform::clock_type::now();

         common::log::internal::transaction << "transaction manager start\n";



         //
         // Connect and get configuration from broker
         //
         {
            trace::internal::Scope trace( "configure", common::log::internal::transaction);
            action::configure( m_state, settings.configuration);
         }

         //
         // Start resource-proxies
         //
         {
            trace::internal::Scope trace( "start rm-proxy-servers", common::log::internal::transaction);

            common::range::for_each(
               m_state.resources,
               action::resource::Instances( m_state));

            //
            // Make sure we wait for the resources to get ready
            //
            common::message::dispatch::Handler handler{
               common::message::handle::Shutdown{},
               handle::resource::reply::Connect{ m_state}};



            while( ! m_state.ready())
            {
               handler( ipc::device().blocking_next( handler.types()));
            }

         }


         auto instances = common::range::accumulate(
               m_state.resources, 0,
               []( std::size_t count, const state::resource::Proxy& p)
               {
                  return count + p.instances.size();
               }
               );

         auto end = common::platform::clock_type::now();


         common::log::information << "transaction manager is on-line - "
               << m_state.resources.size() << " resources - "
               << instances << " instances - boot time: "
               << std::chrono::duration_cast< std::chrono::milliseconds>( end - start).count() << " ms" << std::endl;

      }

      Manager::~Manager()
      {
         common::Trace trace{ "transaction::Manager::~Manager", common::log::internal::transaction};

         try
         {

            for( auto& resource : m_state.resources)
            {
               resource.concurency = 0;
            }

            range::for_each( m_state.resources, action::resource::Instances{ m_state});

            auto processes = m_state.processes();

            process::lifetime::wait( processes, std::chrono::milliseconds( processes.size() * 100));

         }
         catch( ...)
         {
            common::error::handler();
         }

      }

      void Manager::start()
      {
         try
         {
            //
            // We're ready to start....
            //
            message::pump( m_state);

         }
         catch( const common::exception::signal::Terminate&)
         {
            // we do nothing, and let the dtor take care of business
         }
         catch( ...)
         {
            common::error::handler();
         }
      }


      const State& Manager::state() const
      {
         return m_state;
      }

      namespace message
      {
         void pump( State& state)
         {
            try
            {


               common::log::internal::transaction << "prepare message dispatch handlers\n";

               //
               // prepare message dispatch handlers...
               //

               common::message::dispatch::Handler handler{
                  common::message::handle::Shutdown{},
                  handle::dead::Process{ state},
                  handle::Commit{ state},
                  handle::Rollback{ state},
                  handle::resource::Involved{ state},
                  handle::resource::reply::Connect{ state},
                  handle::resource::reply::Prepare{ state},
                  handle::resource::reply::Commit{ state},
                  handle::resource::reply::Rollback{ state},
                  handle::domain::Prepare{ state},
                  handle::domain::Commit{ state},
                  handle::domain::Rollback{ state},
                  handle::domain::resource::reply::Prepare{ state},
                  handle::domain::resource::reply::Commit{ state},
                  handle::domain::resource::reply::Rollback{ state},
                  common::server::handle::basic_admin_call{
                     ipc::device().device(), admin::services( state), common::process::instance::transaction::manager::identity(),
                           ipc::device().error_handler()},
                  common::message::handle::ping(),

                  //
                  // We discard the connect reply message
                  //
                  common::message::handle::Discard< common::message::server::connect::Reply>{},
               };


               common::log::internal::transaction << "start message pump\n";



               persistent::Writer batchWrite( state.log);

               while( true)
               {
                  common::Trace trace{ "transaction::Manager message pump", common::log::internal::transaction};

                  {
                     batchWrite.begin();

                     if( ! state.pending())
                     {
                        //
                        // We can only block if our backlog is empty
                        //

                        //
                        // Removed transaction-timeout from TM, since the semantics are not clear
                        // see commit 559916d9b84e4f84717cead8f2ee7e3d9fd561cd for previous implementation.
                        //
                        handler( ipc::device().blocking_next());

                     }


                     //
                     // Consume until the queue is empty or we've got pending replies equal to batch::transaction
                     // We also do a "busy wait" to try to get more done between each write.
                     //

                     auto count = common::platform::batch::transaction;

                     while( ( handler( ipc::device().non_blocking_next()) || --count > 0 ) &&
                           state.persistentReplies.size() < common::platform::batch::transaction)
                     {
                        ;
                     }
                  }

                  //
                  // Check if we have any persistent stuff to handle, if not we don't do persistent commit
                  //
                  if( ! state.persistentReplies.empty() || ! state.persistentRequests.empty())
                  {
                     batchWrite.commit();

                     //
                     // Send persistent replies to clients
                     //
                     {

                        common::log::internal::transaction << "manager persistent replies: " << state.persistentReplies.size() << "\n";

                        auto notDone = common::range::partition(
                              state.persistentReplies,
                              common::negate( action::persistent::Send{ state}));

                        common::range::trim( state.persistentReplies, std::get< 0>( notDone));

                        common::log::internal::transaction << "manager persistent replies: " << state.persistentReplies.size() << "\n";
                     }

                     //
                     // Send persistent resource requests
                     //
                     {
                        common::log::internal::transaction << "manager persistent request: " << state.persistentRequests.size() << "\n";

                        auto notDone = common::range::partition(
                              state.persistentRequests,
                              common::negate( action::persistent::Send{ state}));

                        //
                        // Move the ones that did not find an idle resource to pending requests
                        //
                        common::range::move( std::get< 0>( notDone), state.pendingRequests);

                        state.persistentRequests.clear();

                     }
                  }
                  common::log::internal::transaction << "manager transactions: " << state.transactions.size() << "\n";
               }
            }
            catch( const exception::Shutdown&)
            {
               //
               // We do nothing
               //
            }
         }

      } // message

   } // transaction
} // casual


