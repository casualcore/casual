

//## includes protected section begin [.10]

#include "broker/adminserverimplementation.h"


#include "broker/broker.h"

#include "broker/transformation.h"


#include "common/trace.h"
#include "common/algorithm.h"



//## includes protected section end   [.10]

namespace casual
{
namespace broker
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]

AdminServerImplementation::AdminServerImplementation( int argc, char **argv)
{
   //## constructor protected section begin [ctor.20]
   //## constructor protected section end   [ctor.20]
 }



AdminServerImplementation::~AdminServerImplementation()
{
   //## destructor protected section begin [dtor.20]
   //## destructor protected section end   [dtor.20]
}


//
// Services definitions
//


std::vector< admin::ServerVO> AdminServerImplementation::_broker_listServers( )
{
   //## service implementation protected section begin [2000]

   auto& state = Broker::instance().state();

   std::vector< admin::ServerVO> result;

   common::range::transform( state.servers, result,
      common::chain::Nested::link(
         admin::transform::Server( state),
         common::extract::Second()));

   return result;


   //## service implementation protected section end   [2000]
}

std::vector< admin::ServiceVO> AdminServerImplementation::_broker_listServices( )
{
   //## service implementation protected section begin [2010]

   auto& state = Broker::instance().state();

   std::vector< admin::ServiceVO> result;

   common::range::transform( state.services, result,
         common::chain::Nested::link(
            admin::transform::Service(),
            common::extract::Second()));

   return result;

   //## service implementation protected section end   [2010]
}

void AdminServerImplementation::_broker_updateInstances( const std::vector<admin::update::InstancesVO>& instances)
{
   common::Trace trace( "AdminServerImplementation::_broker_updateInstances");

   Broker::instance().serverInstances( instances);
}



//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // broker
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]


