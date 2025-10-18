//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/fanout/state.h"
#include "queue/fanout/handle.h"
#include "queue/common/log.h"
#include "queue/common/ipc.h"

#include "casual/argument.h"

#include "common/exception/guard.h"
#include "common/message/dispatch.h"
#include "common/communication/select/ipc.h"
#include "common/communication/instance.h"
#include "common/execute.h"


namespace casual
{
   namespace queue::fanout
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
               // none yet
            };

            auto initialize( Settings settings)
            {
               Trace trace{ "queue::fanout::local::initialize"};

               // connect to QM. QM will send us the initial configuration
               common::communication::device::blocking::send( 
                  ipc::queue::manager(),
                  ipc::message::fanout::group::Connect{ common::process::handle()});
 
               return State{};
            }

            auto condition( State& state)
            {
               return common::message::dispatch::condition::compose(
                  common::message::dispatch::condition::done( [&state](){ return state.done();})
               );
            }


            void start( State state)
            {
               Trace trace{ "queue::fanout::local::start"};

               auto abort_guard = common::execute::scope( [&state]() { fanout::handle::abort( state);});

               common::communication::select::dispatch::pump(
                  local::condition( state),
                  state.directive,
                  state.multiplex,
                  common::communication::select::ipc::dispatch::create( state, &fanout::handle::create));

               abort_guard.release();
            }
            
            void main( int argc, const char** argv)
            {
               Trace trace{ "queue::fanout::local::main"};

               Settings settings;

               argument::parse( "queue fan-out server", {}, argc, argv);

               start( initialize( std::move( settings)));
            }
            
         } // <unnamed>
      } // local

   } // queue::fanout
} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::queue::fanout::local::main( argc, argv);
   });
}
