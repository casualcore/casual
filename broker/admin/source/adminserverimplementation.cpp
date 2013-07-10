

//## includes protected section begin [.10]

#include "broker/adminserverimplementation.h"


#include "broker/broker.h"

#include "broker/transformation.h"


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

   Broker& broker = Broker::instance();

   std::vector< admin::ServerVO> result;

   std::transform(
      std::begin( broker.state().servers),
      std::end( broker.state().servers),
      std::back_inserter( result),
         transform::Chain::link(
            transform::Server(),
            generic::extract::Second()));

   return result;


   //## service implementation protected section end   [2000]
}

std::vector< admin::ServiceVO> AdminServerImplementation::_broker_listServices( )
{
   //## service implementation protected section begin [2010]

   const broker::State& state = Broker::instance().state();

   std::vector< admin::ServiceVO> result;

   std::transform(
      std::begin( state.services),
      std::end( state.services),
      std::back_inserter( result),
         transform::Chain::link(
            transform::Service(),
            generic::extract::Second()));

   return result;

   //## service implementation protected section end   [2010]
}

//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // broker
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
