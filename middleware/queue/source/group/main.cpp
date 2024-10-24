//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/group/state.h"
#include "queue/group/handle.h"
#include "queue/common/ipc/message.h"
#include "queue/common/ipc.h"
#include "queue/common/log.h"

#include "casual/argument.h"
#include "common/process.h"
#include "common/exception/capture.h"
#include "common/communication/instance.h"
#include "common/communication/select/ipc.h"
#include "common/message/signal.h"
#include "common/message/transaction.h"
#include "common/environment.h"
#include "common/instance.h"
#include "common/event/send.h"

#include "sql/database.h"

namespace casual
{
   using namespace common;
   namespace queue::group
   {
         namespace local
         {
            namespace
            {
               namespace select
               {
                  //! Policy for the ipc multiplex select message pump
                  struct Policy
                  {
                     struct next
                     {
                        inline static platform::size::type ipc() noexcept 
                        {
                           auto initialize_value = []()
                           {
                              //! "expose" configuration to be able to tweak and gain knowledge to find a reasonable fixed one.
                              constexpr std::string_view environment = "CASUAL_QUEUE_PERSISTENCE_WINDOW";

                              if( auto value = environment::variable::get< platform::size::type>( environment))
                                 return *value;

                              return platform::size::type{ 20};
                           };

                           static const platform::size::type value = initialize_value();
                           return value;
                        }


                     };
                  };

               } // select

               namespace sqlite::library::version
               {
                  constexpr int required = sql::database::version::number::calculate( 3, 35, 0);

                  void check()
                  {
                     if( sql::database::version::library() < required)
                        common::event::error::fatal::raise( common::code::casual::invalid_version,
                           "casual-queue-group requires sqlite3 version 3.35.0 or greater"
                        );
                  }
               }

               struct Settings
               {
                  CASUAL_LOG_SERIALIZE()
               };

               void connect_with_QM()
               {
                  // connect to queue-manager - it will send configuration::update::Request that we'll handle
                  // in the main message pump
                  communication::device::blocking::send( 
                     communication::instance::outbound::queue::manager::device(),
                     ipc::message::group::Connect{ process::handle()});
               }

               void connect_with_TM()
               {
                  common::message::transaction::resource::external::Instance message{ process::handle()};
                  message.alias = instance::alias();
                  message.description = "queue-group";
                  
                  communication::device::blocking::send( 
                     communication::instance::outbound::transaction::manager::device(),
                     message);
               }

               State initialize( Settings settings)
               {
                  Trace trace{ "queue::group::local::initialize"};

                  sqlite::library::version::check();

                  // make sure we handle "alarms"
                  signal::callback::registration< common::code::signal::alarm>( []()
                  {
                     // Timeout has occurred, we push the corresponding 
                     // signal to our own "queue", and handle it "later"
                     ipc::device().push( common::message::signal::Timeout{});
                  });

                  connect_with_QM();

                  connect_with_TM();

                  // 'connect' to our local domain
                  common::communication::instance::whitelist::connect();

                  return {};
               }

               auto condition( State& state)
               {
                  return common::message::dispatch::condition::compose(
                     common::message::dispatch::condition::done( [&state](){ return state.done();}),
                     common::message::dispatch::condition::idle( [&state]()
                     {
                        // make sure we persist when inbound is idle,
                        handle::persist( state);
                     })
                  );
               }

               void start( State state)
               {
                  Trace trace{ "queue::group::local::start"};

                  auto abort_guard = execute::scope( [&state]()
                  {
                     handle::abort( state);
                  });

                  communication::select::dispatch::pump(
                     local::condition( state),
                     state.directive,
                     state.multiplex,
                     communication::select::ipc::dispatch::create< local::select::Policy>( state, &group::handlers));

                  abort_guard.release();
               }

               void main( int argc, const char** argv)
               {
                  Trace trace{ "queue::group::local::main"};

                  Settings settings;
                  argument::parse( "queue group server", {}, argc, argv);

                  log::line( verbose::log, "settings: ", settings);

                  start( initialize( std::move( settings)));
               }

            } // <unnamed>
         } // local

   } // queue::group
} // casual

int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::queue::group::local::main( argc, argv);
   });
}
