//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "event/service/monitor/vo/entry.h"

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



                  std::vector< vo::Entry> select()
                  {
                     const common::Trace trace( "Database::select");

                     auto connection = sql::database::Connection( common::environment::directory::domain() + "/monitor.db");
                     auto query = connection.query( "SELECT service, parentservice, callid, transactionid, start, end FROM calls;");

                     sql::database::Row row;
                     std::vector< vo::Entry> result;

                     while( query.fetch( row))
                     {
                        vo::Entry vo;
                        vo.setService( row.get< std::string>(0));
                        vo.setParentService( row.get< std::string>( 1));
                        common::Uuid callId( row.get< std::string>( 2));
                        vo.setCallId( callId);
                        //vo.setTransactionId( local::getValue( *row, "transactionid"));

                        std::chrono::microseconds start{ row.get< long long>( 4)};
                        vo.setStart( common::platform::time::point::type{ start});
                        std::chrono::microseconds end{ row.get< long long>( 5)};
                        vo.setEnd( common::platform::time::point::type{ end});
                        result.push_back( vo);
                     }

                     return result;
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

