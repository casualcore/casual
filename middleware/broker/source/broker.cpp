//!
//! casual
//!

#include "broker/broker.h"
#include "broker/handle.h"
#include "broker/transform.h"

#include "broker/admin/server.h"

#include "config/domain.h"

#include "common/environment.h"
#include "common/domain.h"

#include "common/internal/trace.h"
#include "common/internal/log.h"

#include "common/message/dispatch.h"
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


		Broker::Broker( Settings&& arguments)
		{
		   Trace trace{ "Broker::Broker ctor", log::internal::debug};

		   //
		   // Connect to domain
		   //
		   process::instance::connect( process::instance::identity::broker());
		}



		Broker::~Broker()
		{
		   try
		   {
            //
            // Terminate
            //
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
            common::log::internal::debug << "broker has been terminated\n";
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

               common::log::internal::debug << "prepare message-pump handlers\n";


               auto handler = broker::handler( state);

               common::log::internal::debug << "start message pump\n";


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

                        common::log::internal::debug << "pending replies: " << range::make( state.pending.replies) << '\n';

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
                        auto count = common::platform::batch::transaction;

                        while( handler( ipc::device().non_blocking_next()) && count-- > 0)
                           ;
                     }

                  }
               }


            }
            catch( const common::exception::signal::Terminate&)
            {
               // we do nothing, and let the dtor take care of business
               common::log::internal::debug << "broker has been terminated\n";
            }
            catch( ...)
            {
               common::error::handler();
            }
         }

      } // message

	} // broker
} // casual





