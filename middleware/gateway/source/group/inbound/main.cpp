//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/handle.h"
#include "gateway/group/inbound/state.h"
#include "gateway/group/handle.h"
#include "gateway/group/ipc.h"
#include "gateway/group/tcp/listen.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/log/stream.h"
#include "common/communication/instance.h"
#include "common/argument.h"
#include "common/message/internal.h"


namespace casual
{
   namespace gateway::group::inbound
   {
      using namespace common;

      namespace local
      {
         namespace
         {

            struct Arguments
            {
               CASUAL_LOG_SERIALIZE()
            };

            // local state to keep additional stuff for reverse connections...
            struct State : inbound::State
            {
               tcp::listen::state::Listen< configuration::model::gateway::inbound::Connection> listen;
               
               CASUAL_LOG_SERIALIZE(
                  inbound::State::serialize( archive);
                  CASUAL_SERIALIZE( listen);
               )
            };

 

            State initialize( Arguments arguments)
            {
               Trace trace{ "gateway::group::inbound::local::initialize"};
               log::line( verbose::log, "arguments: ", arguments);

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               // TODO multiplex send?
               ipc::flush::send( ipc::manager::gateway(), gateway::message::inbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

               return {};
            }

            namespace internal
            {
               // handles that are specific to the inbound
               namespace handle
               {
                  namespace configuration::update
                  {
                     auto request( State& state)
                     {
                        return [&state]( gateway::message::inbound::configuration::update::Request&& message)
                        {
                           Trace trace{ "gateway::group::inbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;
                           state.pending.requests.limit( message.model.limit);

                           tcp::listen::attempt( state, std::move( message.model.connections));

                           log::line( verbose::log, "state: ", state);

                           // send reply
                           state.multiplex.send( message.process.ipc, common::message::reverse::type( message, common::process::handle()));
                        };
                     }
                  } // configuration::update


                  namespace state
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::inbound::state::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::local::handle::internal::state::request"};
                           
                           state.multiplex.send( message.process.ipc, tcp::listen::state::request( state, message));
                        };
                     }

                  } // state

                  namespace event
                  {
                     namespace process
                     {
                        auto exit( State& state)
                        {
                           return [&state]( common::message::event::process::Exit& message)
                           {
                              Trace trace{ "gateway::group::inbound::local::handle::internal::::event::process::exit"};
                              common::log::line( verbose::log, "message: ", message);

                              // the process might be from our spawned connector
                              state.external.pending().exit( message.state);
                           };
                        }
                     } // process
                  } // event

                  namespace shutdown
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::shutdown::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::local::handle::internal::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           state.listen.clear( state.directive);

                           inbound::handle::shutdown( state);
                        };
                     }
                  } // shutdown

                  namespace connection
                  {
                     auto lost()
                     {
                        return []( message::inbound::connection::Lost message)
                        {
                           Trace trace{ "gateway::group::inbound::local::internal::handle::connection::lost"};
                           log::line( verbose::log, "message: ", message);

                           // we just log the 'event'
                           log::line( log::category::information, code::casual::communication_unavailable, " lost connection to domain: ", message.remote);
                        };
                     }
                  } // connection

               } // handle

               auto handler( State& state)
               {
                  // we add the common/general inbound logic
                  return inbound::handle::internal( state) + common::message::dispatch::handler( ipc::inbound(),
                     common::message::internal::dump::state::handle( state),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::event::process::exit( state),
                     handle::shutdown::request( state),
                     handle::connection::lost()
                  );
               }

            } // internal


            auto condition( State& state)
            {
               return communication::select::dispatch::condition::compose(
                  communication::select::dispatch::condition::done( [&state](){ return state.done();}),
                  communication::select::dispatch::condition::idle( [&state](){ inbound::handle::idle( state);})
                  );
            }

            void run( State state)
            {
               Trace trace{ "gateway::group::inbound::local::run"};
               log::line( verbose::log, "state: ", state);

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::group::inbound::handle::abort( state);
               });

               // make sure we listen to the death of our children
               common::signal::callback::registration< code::signal::child>( group::handle::signal::process::exit());

               {
                  Trace trace{ "gateway::group::inbound::local::run dispatch pump"};

                  // start the message dispatch
                  communication::select::dispatch::pump( 
                     local::condition( state),
                     state.directive,
                     group::tcp::pending::send::dispatch( state),
                     ipc::dispatch::create( state, &internal::handler),
                     tcp::handle::dispatch::create( state, inbound::handle::external( state), &handle::connection::lost),
                     tcp::listen::dispatch::create( state, tcp::logical::connect::Bound::in),
                     state.multiplex
                  );
               }

               abort_guard.release();
            }

            void main( int argc, char** argv)
            {
               Trace trace{ "gateway::group::inbound::local::main"};

               Arguments arguments;

               argument::Parse{ "inbound",
               }( argc, argv);

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::group::inbound

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::gateway::group::inbound::local::main( argc, argv);
   });
} // main