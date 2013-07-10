
		

#include <sf/archive.h>


//## includes protected section begin [400.20]

#include "broker/servicevo.h"

//## includes protected section end   [400.20]

namespace casual
{
namespace broker
{
namespace admin
{



struct ServiceVO::Implementation
{
    Implementation()
   //## initialization list protected section begin [400.40]
   //## initialization list protected section end   [400.40]
   {
      //## ctor protected section begin [400.impl.ctor.10]
      //## ctor protected section end   [400.impl.ctor.10]
   }

 
   template< typename A>
   void serialize( A& archive)
   {
      //## additional serialization protected section begin [400.impl.serial.10]
      //## additional serialization protected section end   [400.impl.serial.10]
      archive & CASUAL_MAKE_NVP( nameF);
      archive & CASUAL_MAKE_NVP( timeoutF);
      archive & CASUAL_MAKE_NVP( pids);
      //## additional serialization protected section begin [400.impl.serial.20]
      //## additional serialization protected section end   [400.impl.serial.20]
   }

   //## additional attributes protected section begin [400.impl.attr.10]
   //## additional attributes protected section end   [400.impl.attr.10]
   std::string nameF;
   long timeoutF;
   std::vector< long> pids;
   //## additional attributes protected section begin [400.impl.attr.20]
   //## additional attributes protected section end   [400.impl.attr.20]

};




ServiceVO::ServiceVO()
   : pimpl( new Implementation())  
{
   //## base class protected section begin [400.ctor.10]
   //## base class protected section end   [400.ctor.10]  
}

ServiceVO::~ServiceVO() = default;

ServiceVO::ServiceVO( ServiceVO&&  rhs) = default;

ServiceVO& ServiceVO::operator = (ServiceVO&&) = default;


ServiceVO::ServiceVO( const ServiceVO& rhs)
   : pimpl( new Implementation( *rhs.pimpl))
{

}

ServiceVO& ServiceVO::operator = ( const ServiceVO& rhs)
{
    *pimpl = *rhs.pimpl;
    return *this;
}

std::string ServiceVO::getNameF() const
{
   return pimpl->nameF;
}
long ServiceVO::getTimeoutF() const
{
   return pimpl->timeoutF;
}
std::vector< long> ServiceVO::getPids() const
{
   return pimpl->pids;
}


void ServiceVO::setNameF( std::string value)
{
   pimpl->nameF = value;
}
void ServiceVO::setTimeoutF( long value)
{
   pimpl->timeoutF = value;
}
void ServiceVO::setPids( std::vector< long> value)
{
   pimpl->pids = value;
}






void ServiceVO::serialize( casual::sf::archive::Reader& archive)
{
    pimpl->serialize( archive);
}
  
void ServiceVO::serialize( casual::sf::archive::Writer& archive) const
{
    pimpl->serialize( archive);
}




} // admin
} // broker
} // casual

	
