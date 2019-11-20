//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!




#include "common/service/invoke.h"
#include "common/service/type.h"
#include "common/environment.h"
#include "common/argument.h"
#include "common/log.h"
#include "common/server/start.h"
#include "common/exception/handle.h"

#include "serviceframework/service/protocol.h"

#include "sql/database.h"

namespace casual
{
   namespace event
   {
      namespace service
      {
         namespace monitor
         {
            namespace local
            {
               namespace
               {
                  namespace database
                  {
                     std::string name = "monitor.db";
                  } // database

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

                  auto select()
                  {
                     const common::Trace trace( "Database::select");

                     auto connection = sql::database::Connection( common::environment::directory::domain() + "/monitor.db");
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
                     common::service::invoke::Result metrics( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        auto result = serviceframework::service::user( protocol, &local::select);

                        protocol << CASUAL_NAMED_VALUE( result);
                        return protocol.finalize();
                     }
                  } // service
               } // <unnamed>
            } // local



            void main( int argc, char **argv)
            {
               // get database from arguments

               {
                  using namespace casual::common::argument;
                  Parse parse{ "service monitor server",
                     Option( std::tie( local::database::name), { "-db", "--database"}, "path to monitor database log")
                  };

                  parse( argc, argv);
               }

               common::server::start( {
                  {
                     ".casual/event/service/metrics",
                     &local::service::metrics,
                     common::service::transaction::Type::none,
                     common::service::category::admin(),
                  }
               });

            }

         } // monitor
      } // service
   } // event
} // casual


int main( int argc, char **argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::event::service::monitor::main( argc, argv);
   });
}

