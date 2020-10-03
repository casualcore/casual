//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/common/log.h"
#include "queue/common/ipc.h"
#include "queue/forward/state.h"
#include "queue/forward/handle.h"

#include "common/message/dispatch.h"
#include "common/exception/guard.h"

namespace casual
{
   using namespace common;

   namespace queue
   {
      namespace forward
      {
         namespace local
         {
            namespace
            {
               void connect()
               {
                  Trace trace{ "queue::forward::concurrent::service::local::connect"};

                  communication::device::blocking::send( 
                     ipc::queue::manager(),
                     message::queue::forward::configuration::Request{ process::handle()});
               }

               auto condition( State& state)
               {
                  return message::dispatch::condition::compose(
                     message::dispatch::condition::done( [&state]()
                     {
                        return state.done();
                     })
                  );
               }


               void start()
               {
                  Trace trace{ "queue::forward::concurrent::service::local::start"};

                  // connect, which will trigger manager to send configuration, that we'll
                  // consume in the dispatch handler.
                  connect();
                  
                  // see State::Pending documentation for the samantics.
                  State state;

                  message::dispatch::pump( condition( state), forward::handlers( state), ipc::device());

                  log::line( verbose::log, "state: ", state);
               }

               void main( int argc, char** argv)
               {
                  Trace trace{ "queue::forward::concurrent::service::local::main"};

                  start();
               
               } // main

            } // <unnamed>
         } // local

      } // forward
   } // queue
} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::queue::forward::local::main( argc, argv);
   });
   
} 