#ifndef CASUALSTATISTICSMONITORSTATISTICIMPLEMENTATION_H
#define CASUALSTATISTICSMONITORSTATISTICIMPLEMENTATION_H


//## includes protected section begin [.10]

#include <vector>

#include "traffic/monitor/serviceentryvo.h"

//## includes protected section end   [.10]

namespace casual
{
namespace traffic
{
namespace monitor
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


class RequestServerImplementation
{

public:
    
   //
   // Constructor and destructor. 
   // use these to initialize state if needed
   //
   RequestServerImplementation( int argc, char **argv);
    
   ~RequestServerImplementation();
    

   //## declarations protected section begin [.200]
   //## declarations protected section end   [.200]
    
    
   //
   // Services
   //
    
   
   //!
   //! Does some stuff 
   //! 
   //!
   std::vector< ServiceEntryVO> getMonitorStatistics();

   //## declarations protected section begin [.300]
   //## declarations protected section end   [.300]

};


//## declarations protected section begin [.40]
//## declarations protected section end   [.40]
} // monitor
} // traffic
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
#endif 
