//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "casual/argument.h"

#include "common/environment.h"
#include "common/log.h"
#include "common/exception/guard.h"
#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"
#include "common/communication/instance.h"

#include "casual/manager/service/protocol.h"
#include "casual/manager/service.h"
#include "casual/manager/service/context.h"
#include "casual/manager/service/policy.h"

#include "sql/database.h"

namespace casual
{
   namespace event::service::monitor
   {

      namespace local
      {
         namespace
         {

            struct State
            {
               std::string database = "monitor.db";
               manager::service::Context< manager::service::policy::Default> services;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( database);
                  CASUAL_SERIALIZE( services);
               )
            };


            namespace model
            {
               struct Entry
               {
                  struct 
                  {
                     std::string name;
                     std::string parent;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( name);
                        CASUAL_SERIALIZE( parent);
                     )
                  } service;

                  common::Uuid execution;
                  platform::time::point::type start;
                  platform::time::point::type end;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( execution);
                     CASUAL_SERIALIZE( start);
                     CASUAL_SERIALIZE( end);
                  )

               };
            } // model

            auto select( const State& state)
            {
               const common::Trace trace( "Database::select");

               auto connection = sql::database::Connection( common::environment::directory::domain() / "monitor.db");
               //auto query = connection.query( "SELECT service, parentservice, callid, transactionid, start, end FROM calls;");
               auto query = connection.query( "SELECT service, parentservice, callid, start, end FROM calls;");

               return sql::database::query::fetch( std::move( query), []( sql::database::Row& row)
               {
                  model::Entry entry;
                  sql::database::row::get( row, 
                     entry.service.name,
                     entry.service.parent,
                     entry.execution.get(),
                     entry.start,
                     entry.end);

                  return entry;
               });
            }

            namespace service
            {
               auto metrics( const State& state)
               {
                  return [ &state]( casual::manager::service::invoke::Parameter&& parameter)
                  {
                     return casual::manager::service::protocol::dispatch( 
                        std::move( parameter),
                        &local::select,
                        state);
                  };
               }
            } // service

            std::vector< casual::manager::Service> services( const State& state)
            {
               return { 
                  { 
                     .name = std::string{ ".casual/event/service/metrics"},
                     .function = local::service::metrics( state),
                     .visibility = common::service::visibility::Type::undiscoverable,
                     .category = std::string{ common::service::category::admin}
                  }
               };
            }

            auto handlers( State& state)
            {
               return common::message::dispatch::handle::defaults( state) +
                  state.services.initialize( local::services( state));

            }

            void start( State& state)
            {
               common::Trace trace{ "event::service::monitor::start"};

               // Start the message-pump
               common::message::dispatch::pump(
                  local::handlers( state),
                  common::communication::ipc::inbound::device());
            }

         } // <unnamed>
      } // local



      void main( int argc, const char** argv)
      {
         // get database from arguments

         local::State state;

         {
            argument::parse( "service monitor server",{
               argument::Option( std::tie( state.database), { "-db", "--database"}, "path to monitor database log")
            }, argc, argv);
         }

         local::start( state);
      }

   } // event::service::monitor
} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [&]()
   {
      casual::event::service::monitor::main( argc, argv);
   });
}

