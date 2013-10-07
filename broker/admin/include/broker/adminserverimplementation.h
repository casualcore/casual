#ifndef CASUALBROKERADMINSERVERIMPLEMENTATION_H
#define CASUALBROKERADMINSERVERIMPLEMENTATION_H


//## includes protected section begin [.10]

#include <vector>
#include <string>


#include "broker/brokervo.h"


//## includes protected section end   [.10]

namespace casual
{
namespace broker
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


class AdminServerImplementation
{

public:
    
   //
   // Constructor and destructor. 
   // use these to initialize state if needed
   //
   AdminServerImplementation( int argc, char **argv);
    
   ~AdminServerImplementation();
    

   //## declarations protected section begin [.200]
   //## declarations protected section end   [.200]
    
    
   //
   // Services
   //
    
   
   //!
   //! @return a list of all servers
   //!
   std::vector< admin::ServerVO> _broker_listServers( );

   //!
   //! @return a list of all services
   //!
   std::vector< admin::ServiceVO> _broker_listServices( );

   //!
   //!
   //!
   std::vector< admin::ServerVO> _broker_updateInstances( const std::vector< admin::update::InstancesVO>& instances);

   //## declarations protected section begin [.300]
   //## declarations protected section end   [.300]

};


//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // broker
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
#endif 
