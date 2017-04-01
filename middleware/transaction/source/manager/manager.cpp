//!
//! casual
//!

#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/manager/admin/server.h"
#include "transaction/common.h"


#include "common/server/handle/call.h"
#include "common/environment.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/log.h"

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

      Manager::Manager( const Settings& settings) :
          m_state( settings.log)
      {
         auto start = common::platform::time::clock::type::now();

         log << "transaction manager start\n";


         //
         // Connect to domain
         //
         process::instance::connect( process::instance::identity::transaction::manager());

         //
         // get configuration from domain manager
         //
         action::configure( m_state);


         //
         // Start resource-proxies
         //
         {
            Trace trace{ "start rm-proxy-servers"};

            common::range::for_each(
               m_state.resources,
               action::resource::Instances( m_state));

            //
            // Make sure we wait for the resources to get ready
            //
            auto handler = ipc::device().handler(
               common::message::handle::Shutdown{},
               handle::resource::reply::Connect{ m_state});



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

         auto end = common::platform::time::clock::type::now();


         common::log::category::information << "transaction manager is on-line - "
               << m_state.resources.size() << " resources - "
               << instances << " instances - boot time: "
               << std::chrono::duration_cast< std::chrono::milliseconds>( end - start).count() << " ms" << std::endl;

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


               log << "prepare message dispatch handlers\n";

               //
               // prepare message dispatch handlers...
               //

               auto handler = ipc::device().handler(
                  common::message::handle::Shutdown{},
                  handle::process::Exit{ state},
                  handle::Commit{ state},
                  handle::Rollback{ state},
                  handle::resource::Involved{ state},
                  handle::resource::reply::Connect{ state},
                  handle::resource::reply::Prepare{ state},
                  handle::resource::reply::Commit{ state},
                  handle::resource::reply::Rollback{ state},
                  handle::external::Involved{ state},
                  handle::domain::Prepare{ state},
                  handle::domain::Commit{ state},
                  handle::domain::Rollback{ state},
                  common::server::handle::basic_admin_call{
                     admin::services( state),
                     ipc::device().error_handler()},
                  common::message::handle::ping()
               );


               log << "start message pump\n";



               persistent::Writer batchWrite( state.log);

               while( true)
               {
                  Trace trace{ "transaction::Manager message pump"};

                  {
                     batchWrite.begin();

                     if( ! state.outstanding())
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

                     auto count = common::platform::batch::transaction();

                     while( ( handler( ipc::device().non_blocking_next()) || --count > 0 ) &&
                           state.persistent.replies.size() < common::platform::batch::transaction())
                     {
                        ;
                     }
                  }

                  //
                  // Check if we have any persistent stuff to handle, if not we don't do persistent commit
                  //
                  if( ! state.persistent.replies.empty() || ! state.persistent.requests.empty())
                  {
                     batchWrite.commit();

                     //
                     // Send persistent replies to clients
                     //
                     {

                        log << "manager persistent replies: " << state.persistent.replies.size() << "\n";

                        auto not_done = common::range::partition(
                              state.persistent.replies,
                              common::negate( action::persistent::Send{ state}));

                        common::range::trim( state.persistent.replies, std::get< 0>( not_done));

                        log << "manager persistent replies: " << state.persistent.replies.size() << "\n";
                     }

                     //
                     // Send persistent resource requests
                     //
                     {
                        log << "manager persistent request: " << state.persistent.requests.size() << "\n";

                        auto not_done = common::range::partition(
                              state.persistent.requests,
                              common::negate( action::persistent::Send{ state}));

                        //
                        // Move the ones that did not find an idle resource to pending requests
                        //
                        common::range::move( std::get< 0>( not_done), state.pending.requests);

                        state.persistent.requests.clear();

                     }
                  }
                  log << "manager transactions: " << state.transactions.size() << "\n";
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


