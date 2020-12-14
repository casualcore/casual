//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/common.h"
#include "domain/pending/message/environment.h"
#include "domain/pending/message/message.h"

#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/message/handle.h"
#include "common/exception/handle.h"
#include "common/signal/timer.h"

namespace casual
{
   using namespace common;

   namespace domain
   {      
      namespace pending
      {
         namespace message
         {
            namespace local
            {
               namespace
               {
                  struct State 
                  {
                     bool done = false;
                     std::vector< message::Request> messages;

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( done);
                        CASUAL_SERIALIZE( messages);
                     )
                  };

                  namespace update
                  {
                     void timout( State& state)
                     {
                        if( state.messages.empty())
                           signal::timer::unset();
                        else if( ! signal::timer::get())
                           signal::timer::set( std::chrono::milliseconds{ 500});
                     }

                  } // update

                  
                  namespace ipc
                  {
                     namespace messages
                     {
                        void send( State& state)
                        {
                           Trace trace{ "domain::pending::message::local::ipc::messages::send"};

                           // we block all signals during non-blocking-send
                           signal::thread::scope::Block block;

                           auto send = []( message::Request& request)
                           {
                              return common::message::pending::non::blocking::send( request.message);
                           };

                           common::algorithm::trim( state.messages, common::algorithm::remove_if( state.messages, send));

                           log::line( verbose::log, "state: ", state);

                           update::timout( state);
                        }     
                     } // messages

                  } // ipc

                  namespace callback
                  {
                     auto timeout( State& state)
                     {
                        return [&state]()
                        {
                           Trace trace{ "domain::pending::message::local::callback::timeout"};
                           
                           ipc::messages::send( state);
                        };
                     }
                  } // callback
                  
                  namespace handle
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::Request& message)
                        {
                           Trace trace{ "domain::pending::message::local::handle::request"};
                           log::line( verbose::log, "message: ", message);
                           
                           // we block all signals during non-blocking-send
                           signal::thread::scope::Block block;

                           // we add it, and try to send all messages
                           state.messages.push_back( std::move( message));

                           ipc::messages::send( state);
                        };
                     }

                     auto shutdown( State& state)
                     {
                        return [&state]( const common::message::shutdown::Request&)
                        {
                           Trace trace{ "domain::pending::message::local::handle::shutdown"};

                           state.done = true;

                           // we try to send any pending that is left...
                           ipc::messages::send( state);

                           if( ! state.messages.empty())
                              log::line( log::category::error, "pending messages not sent: ", state.messages);
                        };
                     }

                  } // handle

                     
                  void start()
                  {
                     Trace trace{ "domain::pending::message::local::start"};

                     State state;

                     // register the alarm callback.
                     signal::callback::registration< code::signal::alarm>( callback::timeout( state));

                     auto& ipc = communication::ipc::inbound::device();

                     // connect process
                     {
                        pending::message::connect::Request request{ common::process::handle()};
                        request.identification = message::environment::identification;
                        
                        auto reply = communication::ipc::call( communication::instance::outbound::domain::manager::device(), request);

                        if( reply.directive != decltype( reply.directive)::start)
                        {
                           log::line( log::category::error, "domain manager said no to start: ", reply.directive);
                           return;
                        }

                     }
                     

                     auto handler = common::message::dispatch::handler( ipc,
                        common::message::handle::defaults( ipc),
                        handle::request( state),
                        handle::shutdown( state));

                     auto condition = common::message::dispatch::condition::compose( 
                        common::message::dispatch::condition::done( [&state](){ return state.done;})
                     );

                     common::message::dispatch::pump(
                        std::move( condition),
                        handler,
                        communication::ipc::inbound::device());
                  }

               } // <unnamed>
            } // local

            int main( int argc, const char **argv)
            {
               return common::exception::main::guard( []()
               {
                  local::start();
               });
            }
         } // message
      } // pending
   } // domain
} // casual

int main( int argc, const char **argv)
{
   return casual::domain::pending::message::main( argc, argv);
}

