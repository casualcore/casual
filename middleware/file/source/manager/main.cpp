//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/manager/state.h"
#include "file/manager/handle.h"
#include "file/instance.h"

#include "casual/argument.h"
#include "common/log.h"
#include "common/exception/guard.h"
#include "common/communication/select/ipc.h"
#include "common/communication/instance.h"



namespace casual
{
   namespace file::manager
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
            };

            State initialize( Settings settings)
            {
               State result;

               common::communication::instance::whitelist::connect( file::instance::identity);

               return result;
            }

            auto condition( State& state)
            {
               return common::message::dispatch::condition::compose(
                  common::message::dispatch::condition::done( [ &state]() { return state.done();})
               );
            }

            void start( State state)
            {
               common::Trace trace{ "file::manager::local::start"};

               common::communication::select::dispatch::pump(
                  local::condition( state),
                  state.directive,
                  state.multiplex,
                  common::communication::select::ipc::dispatch::create( state, &handle::create));

            }

            void main( int argc, char **argv)
            {
               Settings settings;

               casual::argument::parse( R"(Manages casual file, the provided file functionality)", {}, argc, argv);

               start( initialize( std::move( settings)));
            }

         } //
      } // local
         
   } // file::manager
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::file::manager::local::main( argc, argv);
   });
}


