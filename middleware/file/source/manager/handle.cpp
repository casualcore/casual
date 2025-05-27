//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/manager/handle.h"

#include "file/message.h"
#include "file/manager/resource.h"
#include "file/manager/admin/server.h" 

#include "common/message/type.h"
#include "common/message/transaction.h"
#include "common/event/listen.h"


namespace casual
{
   namespace file::manager::handle
   {
      namespace local
      {
         namespace
         {
            namespace shutdown
            {
               auto request( State& state)
               {
                  return [ &state]( const common::message::shutdown::Request& message)
                  {
                     common::Trace trace{ "file::manager::handle::local::shutdown::request"};
                     common::log::debug( "message: ", message);
                     common::log::debug( "state: ", state);

                     resource::shutdown( state, message);
                  };
               }
            } // shutdown

            namespace reserve
            {
               auto request( State& state)
               {
                  return [ &state]( const file::message::reserve::Request& message)
                  {
                     common::Trace trace{ "file::manager::handle::local::reserve::request"};
                     common::log::debug( "message: ", message);
                     common::log::debug( "state: ", state);

                     resource::reserve( state, message);
                  };
               }
            } // reserve

            namespace transaction
            {
               namespace prepare
               {
                  auto request( State& state)
                  {
                     return [ &state]( const common::message::transaction::resource::prepare::Request& message)
                     {
                        common::Trace trace{ "file::manager::handle::local::transaction::prepare::request"};

                        resource::prepare( state, message);
                     };
                  }
               } // prepare

               namespace commit
               {
                  auto request( State& state)
                  {
                     return [ &state]( const common::message::transaction::resource::commit::Request& message)
                     {
                        common::Trace trace{ "file::manager::handle::local::transaction::commit::request"};

                        resource::commit( state, message);
                     };
                  }
               } // commit

               namespace rollback
               {
                  auto request( State& state)
                  {
                     return [ &state]( const common::message::transaction::resource::rollback::Request& message)
                     {
                        common::Trace trace{ "file::manager::handle::local::transaction::rollback::request"};

                        resource::rollback( state, message);
                     };
                  }
               } // rollback
               
            } // transaction

            namespace event
            {
               namespace process
               {
                  auto exit( State& state)
                  {
                     return [ &state]( const common::message::event::process::Exit& message)
                     {
                        common::Trace trace{ "file::manager::handle::local::event::process::exit"};
                        common::log::debug( "message: ", message);
                        common::log::debug( "state: ", state);

                        resource::mitigate( state, message);
                     };
                  }
               } // process
            } // event

            namespace service::manager::lookup
            {
               auto reply( State& state)
               {
                  return [ &state]( const common::message::domain::process::lookup::Reply& message)
                  {
                     common::Trace trace{ "discovery::handle::local::service::manager::lookup::reply"};
                     common::log::debug( "message: ", message);

                     // will advertise our services if the reply refers to SM.
                     state.services.advertise( message);
                  };

               }
            } // service::manager::lookup
            
         } // <unnamed>
      } // local

      dispatch_type create( State& state)
      {
         return dispatch_type{
            common::event::listener( 
               local::event::process::exit( state)),

            common::message::dispatch::handle::defaults( state),
            
            local::shutdown::request( state),
            local::reserve::request( state),
            local::transaction::prepare::request( state),
            local::transaction::commit::request( state),
            local::transaction::rollback::request( state),
            state.services.initialize( manager::admin::services( state)),
            // take care of the lookup reply for SM that initialize has requested above
            local::service::manager::lookup::reply( state),
         };
      }

   } // file::manager::handle
   
} // casual