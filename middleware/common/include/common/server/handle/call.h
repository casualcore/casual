//!
//! casual
//!

#ifndef CASUAL_COMMON_SESRVER_HANDLE_H_
#define CASUAL_COMMON_SESRVER_HANDLE_H_


#include "common/server/handle/service.h"
#include "common/server/argument.h"
#include "common/server/handle/policy.h"

#include "common/service/call/context.h"


namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace handle
         {



            //!
            //! Handles XATMI-calls
            //!
            //! Semantics:
            //! - construction
            //! -- send connect to broker - connect server - advertise all services
            //! - dispatch
            //! -- set longjump
            //! -- call user XATMI-service
            //! -- when user calls tpreturn we longjump back
            //! -- send reply to caller
            //! -- send ack to broker
            //! -- send time-stuff to monitor (if active)
            //! -- transaction stuff
            //! - destruction
            //! -- send disconnect to broker - disconnect server - unadvertise services
            //!
            //! @note it's a template so we can use the same implementation in casual-broker and
            //!    others that need's another policy (otherwise it would send messages to it self, and so on)
            //!
            template< typename P>
            struct basic_call
            {
               typedef P policy_type;

               typedef message::service::call::callee::Request message_type;

               basic_call( basic_call&&) = default;
               basic_call& operator = ( basic_call&&) = default;

               basic_call() = delete;
               basic_call( const basic_call&) = delete;
               basic_call& operator = ( basic_call&) = delete;




               //!
               //! Connect @p server to the broker, broker will build a dispatch-table for
               //! coming XATMI-calls
               //!
               template< typename... Args>
               basic_call( server::Arguments arguments, Args&&... args)
                  : m_policy( std::forward< Args>( args)...)
               {
                  Trace trace{ "server::handle::basic_call::basic_call"};

                  server::Context::instance().configure( arguments);

                  //
                  // Connect to casual
                  //
                  m_policy.configure( arguments);

                  //
                  // Call tpsrvinit
                  //
                  if( arguments.server_init( arguments.argc, arguments.argv) == -1)
                  {
                     throw exception::NotReallySureWhatToNameThisException( "service init failed");
                  }
               }


               //!
               //! Sends a message::server::Disconnect to the broker
               //!
               ~basic_call() noexcept
               {
                  if( ! m_moved)
                  {
                     try
                     {
                        auto& state = server::Context::instance().state();

                        //
                        // Call tpsrvdone
                        //
                        state.server_done();
                     }
                     catch( ...)
                     {
                        error::handler();
                     }
                  }

               }

               void operator () ( message_type& message)
               {
                  Trace trace{ "server::handle::basic_call::operator()"};

                  log::debug << "message: " << message << '\n';

                  try
                  {
                     service::call( m_policy, common::service::call::Context::instance(), message);
                  }
                  catch( ...)
                  {
                     error::handler();
                  }
               }

               policy_type m_policy;
               move::Moved m_moved;
            };


            //!
            //! Handle service calls from other proceses and does a dispatch to
            //! the register XATMI functions.
            //!
            typedef basic_call< policy::call::Default> Call;

            using basic_admin_call = basic_call< policy::call::Admin>;


         } // handle

      } // server
   } // common
} // casual

#endif // HANDLE_H_
