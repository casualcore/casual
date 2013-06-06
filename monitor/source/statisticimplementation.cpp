

//## includes protected section begin [.10]

#include "monitor/statisticimplementation.h"
#include "monitor/monitordb.h"



//## includes protected section end   [.10]

namespace casual
{
namespace statistics
{
namespace monitor
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]

StatisticImplementation::StatisticImplementation( int argc, char **argv)
{
   //## constructor protected section begin [ctor.20]
   //## constructor protected section end   [ctor.20]
 }



StatisticImplementation::~StatisticImplementation()
{
   //## destructor protected section begin [dtor.20]
   //## destructor protected section end   [dtor.20]
}


//
// Services definitions
//


bool StatisticImplementation::getMonitorStatistics( std::vector< vo::MonitorVO>& outputValues)
{
   //## service implementation protected section begin [2000]

   static MonitorDB monitordb;

   /*
     std::vector< vo::MonitorVO> result;
     result = monitordb.select();

     std::copy( result.begin(), result.end(), std::back_inserter( outputValues));
     */

   outputValues = monitordb.select();

      return true;

   //## service implementation protected section end   [2000]
}

//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // monitor
} // statistics
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
