

//## includes protected section begin [.10]

#include "traffic/monitor/request_server_implementation.h"

#include "sql/database.h"

#include "common/trace.h"
#include "common/environment.h"

//## includes protected section end   [.10]

namespace casual
{
namespace traffic
{
namespace monitor
{


//## declarations protected section begin [.20]
namespace local
{
namespace database
{
namespace
{
   std::vector< ServiceEntryVO> select( )
   {
      const common::Trace trace( "Database::select");

      auto connection = sql::database::Connection( common::environment::directory::domain() + "/monitor.db");
      std::ostringstream stream;
      stream << "SELECT service, parentservice, callid, transactionid, start, end FROM calls;";
      sql::database::Statement::Query query = connection.query( stream.str());

      sql::database::Row row;
      std::vector< ServiceEntryVO> result;

      while( query.fetch( row))
      {
         ServiceEntryVO vo;
         vo.setService( row.get< std::string>(0));
         vo.setParentService( row.get< std::string>( 1));
         sf::platform::Uuid callId( row.get< std::string>( 2));
         vo.setCallId( callId);
         //vo.setTransactionId( local::getValue( *row, "transactionid"));

         std::chrono::microseconds start{ row.get< long long>( 4)};
         vo.setStart( common::platform::time_point{ start});
         std::chrono::microseconds end{ row.get< long long>( 5)};
         vo.setEnd( common::platform::time_point{ end});
         result.push_back( vo);
      }

      return result;

   }
}
}
}
//## declarations protected section end   [.20]

RequestServerImplementation::RequestServerImplementation( int argc, char **argv)
{
   //## constructor protected section begin [ctor.20]
   //## constructor protected section end   [ctor.20]
}



RequestServerImplementation::~RequestServerImplementation()
{
   //## destructor protected section begin [dtor.20]
   //## destructor protected section end   [dtor.20]
}


//
// Services definitions
//


bool RequestServerImplementation::getMonitorStatistics( std::vector< ServiceEntryVO>& outputValues)
{
   //## service implementation protected section begin [2000]


   std::vector< ServiceEntryVO> result;
   result = local::database::select();

   std::copy( result.begin(), result.end(), std::back_inserter( outputValues));

   return true;

   //## service implementation protected section end   [2000]
}

//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // monitor
} // traffic
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
