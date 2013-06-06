#ifndef CASUALSTATISTICSMONITORSTATISTICIMPLEMENTATION_H
#define CASUALSTATISTICSMONITORSTATISTICIMPLEMENTATION_H


//## includes protected section begin [.10]

#include "monitor/monitorvo.h"

#include <vector>

//## includes protected section end   [.10]

namespace casual
{
namespace statistics
{
namespace monitor
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


class StatisticImplementation
{

public:
    
   //
   // Constructor and destructor. 
   // use these to initialize state if needed
   //
   StatisticImplementation( int argc, char **argv);
    
   ~StatisticImplementation();
    

   //## declarations protected section begin [.200]
   //## declarations protected section end   [.200]
    
    
   //
   // Services
   //
    
   
   //!
   //! Does some stuff 
   //! 
   //! @return true if some condition is met
   //! @param values holds some values
   //!
   bool getMonitorStatistics( std::vector< vo::MonitorVO>& outputValues);

   //## declarations protected section begin [.300]
   //## declarations protected section end   [.300]

};


//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // monitor
} // statistics
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
#endif 
