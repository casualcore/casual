//!
//! casual
//!

#include "broker/broker.h"
#include "broker/handle.h"
#include "broker/transform.h"
#include "broker/common.h"

#include "broker/admin/server.h"

#include "common/environment.h"
#include "common/domain.h"

#include "common/internal/trace.h"
#include "common/internal/log.h"

#include "common/message/dispatch.h"
#include "common/message/domain.h"
#include "common/message/handle.h"
#include "common/process.h"
#include "common/domain.h"


#include "sf/log.h"


#include <xatmi.h>

#include <fstream>
#include <algorithm>
#include <iostream>





namespace casual
{
   using namespace common;

	namespace broker
	{


	   namespace local
      {
         namespace
         {


            namespace configure
            {
               std::string forward( const Settings& settings)
               {
                  if( settings.forward.empty())
                  {
                     return environment::directory::casual() + "/bin/casual-forward-cache";
                  }
                  return settings.forward;
               }

               common::message::domain::configuration::service::Manager domain()
               {
                  common::message::domain::configuration::Request request;
                  request.process = process::handle();

                  return communication::ipc::call( communication::ipc::domain::manager::device(), request).domain.service;
               }

               void services( State& state, const common::message::domain::configuration::service::Manager& configuration)
               {
                  Trace trace{ "broker::local::configure::services"};

                  state.default_timeout = configuration.default_timeout;

                  for( auto& config : configuration.services)
                  {
                     common::message::service::call::Service service;

                     service.timeout = config.timeout;
                     service.name = config.name;

                     state.services.emplace( std::make_pair( config.name, std::move( service)));

                  }

               }

               State state( const Settings& settings)
               {
                  State state;

                  //
                  // Connect to domain
                  //
                  process::instance::connect( process::instance::identity::broker());


                  //
                  // Get configuration from domain-manager
                  //
                  auto configuration = domain();


                  configure::services( state, configuration);



                  //
                  // Register for process termination
                  //
                  {
                     Trace trace{ "broker::configure process::termination"};

                     common::message::domain::process::termination::Registration message;
                     message.process = common::process::handle();

                     ipc::device().blocking_send( communication::ipc::domain::manager::device(), message);
                  }

                  //
                  // Start forward
                  //
                  {
                     Trace trace{ "broker::configure spawn forward"};

                     state.forward.pid = common::process::spawn( forward( settings), {});

                     state.forward = common::process::instance::fetch::handle(
                           common::process::instance::identity::forward::cache());

                     log << "forward: " << state.forward << '\n';

                  }

                  return state;
               }

            } // configure

         } // <unnamed>
      } // local



		Broker::Broker( Settings&& settings) : m_state{ local::configure::state( std::move( settings))}
		{
		   Trace trace{ "Broker::Broker ctor"};

		}



		Broker::~Broker()
		{
		   try
		   {
		      Trace trace{ "Broker::Broker dtor"};

            //
            // Terminate
            //
		      process::terminate( m_state.forward);
		   }
         catch( ...)
         {
            common::error::handler();
         }
		}


      void Broker::start()
      {
         try
         {
            message::pump( m_state);
         }
         catch( const common::exception::signal::Terminate&)
         {
            // we do nothing, and let the dtor take care of business
            log << "broker has been terminated\n";
         }
         catch( ...)
         {
            common::error::handler();
         }
		}



      namespace message
      {
         void pump( State& state)
         {
            try
            {
               //
               // Prepare message-pump handlers
               //

               log << "prepare message-pump handlers\n";


               auto handler = broker::handler( state);

               log << "start message pump\n";


               while( true)
               {
                  if( state.pending.replies.empty())
                  {
                     handler( ipc::device().blocking_next());
                  }
                  else
                  {
                     signal::handle();
                     signal::thread::scope::Block block;

                     //
                     // Send pending replies
                     //
                     {

                        log << "pending replies: " << range::make( state.pending.replies) << '\n';

                        decltype( state.pending.replies) replies;
                        std::swap( replies, state.pending.replies);

                        auto remain = std::get< 1>( common::range::partition(
                              replies,
                              common::message::pending::sender(
                                    communication::ipc::policy::non::Blocking{},
                                    ipc::device().error_handler())));

                        range::move( remain, state.pending.replies);
                     }

                     //
                     // Take care of broker dispatch
                     //
                     {
                        //
                        // If we've got pending that is 'never' sent, we still want to
                        // do a lot of broker stuff. Hence, if we got into an 'error state'
                        // we'll still function...
                        //
                        // TODO: Should we have some sort of TTL for the pending?
                        //
                        auto count = common::platform::batch::transaction();

                        while( handler( ipc::device().non_blocking_next()) && count-- > 0)
                           ;
                     }

                  }
               }


            }
            catch( const common::exception::signal::Terminate&)
            {
               // we do nothing, and let the dtor take care of business
               log << "broker has been terminated\n";
            }
            catch( ...)
            {
               common::error::handler();
            }
         }

      } // message

	} // broker
} // casual





