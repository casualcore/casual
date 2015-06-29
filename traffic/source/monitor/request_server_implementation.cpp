

//## includes protected section begin [.10]

#include "traffic/monitor/request_server_implementation.h"

#include "traffic/monitor/database.h"



//## includes protected section end   [.10]

namespace casual
{
namespace traffic
{
namespace monitor
{


//## declarations protected section begin [.20]
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

   static traffic::monitor::Database database;

   std::vector< ServiceEntryVO> result;
   result = database.select();

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
