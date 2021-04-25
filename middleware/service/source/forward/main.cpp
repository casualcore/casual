//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/forward/instance.h"
#include "service/forward/handle.h"
#include "service/forward/state.h"

#include "service/common.h"

#include "common/exception/guard.h"
#include "common/argument.h"

namespace casual
{
   using namespace common;

   namespace service::forward
   {
      namespace local
      {
         namespace
         {
            auto initialize()
            {
               Trace trace{ "service::forward::initialize"};
               State result;

               communication::instance::whitelist::connect( instance::identity);

               return result;
            }

            auto condition( State& state)
            {
               return message::dispatch::condition::compose(
                  message::dispatch::condition::done( [&state]() { return state.done();})
               );
            }

            void start( State state)
            {
               Trace trace{ "service::forward::start"};
               log::line( verbose::log, "state: ", state);

               auto handler = handle::create( state);

               message::dispatch::pump( 
                  local::condition( state),
                  handler, 
                  communication::ipc::inbound::device());
            }

            void main( int argc, char **argv)
            {
               argument::Parse{ "service forward"}( argc, argv);
         
               start( initialize());
            }
         } // <unnamed>
      } // local
 
   } // service::forward
} // casual



int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::service::forward::local::main( argc, argv);
   });
}
