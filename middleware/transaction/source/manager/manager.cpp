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

      Manager::Manager( manager::Settings settings) :
          m_state{ manager::action::state( std::move( settings))}
      {
         log::line( log, "transaction manager start");

         // make sure we handle death of our children
         signal::callback::registration< code::signal::child>( []()
         {
            algorithm::for_each( process::lifetime::ended(), []( auto& exit)
            {
               manager::handle::process::exit( exit);
            });
         }); 

         // Set the process variables so children can communicate with us.
         common::environment::variable::process::set(
            common::environment::variable::name::ipc::transaction::manager,
            common::process::handle());

         // Start resource-proxies
         {
            Trace trace{ "start rm-proxy-servers"};

            common::algorithm::for_each(
               m_state.resources,
               manager::action::resource::Instances( m_state));

            // Make sure we wait for the resources to get ready
            namespace dispatch = common::message::dispatch;
            dispatch::pump( 
               dispatch::condition::compose( dispatch::condition::done( [&](){ return m_state.booted();})),
               manager::handle::startup::handlers( m_state),
               manager::ipc::device());
         }

         log::line( log::category::information, "transaction-manager is on-line");
      }

      Manager::~Manager()
      {
         common::Trace trace{ "transaction::Manager::~Manager"};

         exception::guard( [&]()
         {
            auto scale_down = [&]( auto& resource)
            { 
               exception::guard( [&](){
                  resource.concurency = 0;
                  manager::action::resource::Instances{ m_state}( resource);
               }); 
            };

            algorithm::for_each( m_state.resources, scale_down);

            auto processes = m_state.processes();
            process::lifetime::wait( processes, std::chrono::milliseconds( processes.size() * 100));
         });
      }

      void Manager::start()
      {
         Trace trace{ "transaction::Manager::start"};

         // prepare message dispatch handlers...
         auto handler = manager::handle::handlers( m_state);

         // Connect to domain
         communication::instance::connect( communication::instance::identity::transaction::manager);

         auto idle_callback = [&]()
         {
            // if input device is 'idle', we persist and send persistent replies, if any.
            manager::handle::persist::send( m_state);
         };

         namespace dispatch = common::message::dispatch;

         dispatch::pump(
            dispatch::condition::compose( dispatch::condition::idle( std::move( idle_callback))),
            handler,
            manager::ipc::device());

      }

      const manager::State& Manager::state() const
      {
         return m_state;
      }

   } // transaction
} // casual


