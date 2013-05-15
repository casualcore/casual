//!
//! template_sf_server_implementation.h
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#ifndef MONITOR_SF_SERVER_IMPLEMENTATION_H_
#define MONITOR_SF_SERVER_IMPLEMENTATION_H_


#include "monitor/monitor_vo.h"

#include <vector>


namespace casual
{
namespace statistics
{
namespace monitor
{
      class ServerImplementation
      {
      public:
         ServerImplementation( int argc, char** argv);
         ~ServerImplementation();


         bool getMonitorStatistics( std::vector< vo::MonitorVO>& outputValues);

      private:

      };


}
}
}




#endif /* MONITOR_SF_SERVER_IMPLEMENTATION_H_ */
