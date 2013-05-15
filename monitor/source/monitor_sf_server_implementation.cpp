//!
//! template_sf_server_implementation.cpp
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!


#include "monitor/monitor_sf_server_implementation.h"
#include "monitor/monitordb.h"

namespace casual
{
namespace statistics
{
namespace monitor
{

   ServerImplementation::ServerImplementation( int argc, char** argv)
   {
   }

   ServerImplementation::~ServerImplementation()
   {

   }

   bool ServerImplementation::getMonitorStatistics( std::vector< vo::MonitorVO>& outputValues)
   {
	  static MonitorDB monitordb;

	  std::vector< vo::MonitorVO> result;
	  result = monitordb.select();

	  std::copy( result.begin(), result.end(), std::back_inserter( outputValues));
      return true;

   }

}

}
}



