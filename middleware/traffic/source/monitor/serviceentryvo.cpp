
		

#include "traffic/monitor/serviceentryvo.h"

#include <sf/archive/archive.h>


//## includes protected section begin [200.20]


//## includes protected section end   [200.20]

namespace casual
{
namespace traffic
{
namespace monitor
{



struct ServiceEntryVO::Implementation
{
    Implementation()
   //## initialization list protected section begin [200.40]
   //## initialization list protected section end   [200.40]
   {
      //## ctor protected section begin [200.impl.ctor.10]
      //## ctor protected section end   [200.impl.ctor.10]
   }

 
   template< typename A>
   void serialize( A& archive)
   {
      //## additional serialization protected section begin [200.impl.serial.10]
      //## additional serialization protected section end   [200.impl.serial.10]
      archive & CASUAL_MAKE_NVP( parentService);
      archive & CASUAL_MAKE_NVP( service);
      archive & CASUAL_MAKE_NVP( callId);
      archive & CASUAL_MAKE_NVP( start);
      archive & CASUAL_MAKE_NVP( end);
      //## additional serialization protected section begin [200.impl.serial.20]
      //## additional serialization protected section end   [200.impl.serial.20]
   }

   //## additional attributes protected section begin [200.impl.attr.10]
   //## additional attributes protected section end   [200.impl.attr.10]
   std::string parentService;
   std::string service;
   sf::platform::Uuid callId;
   sf::platform::time_point start;
   sf::platform::time_point end;
   //## additional attributes protected section begin [200.impl.attr.20]
   //## additional attributes protected section end   [200.impl.attr.20]

};




ServiceEntryVO::ServiceEntryVO()
{
   //## base class protected section begin [200.ctor.10]
   //## base class protected section end   [200.ctor.10]  
}

ServiceEntryVO::~ServiceEntryVO() = default;
ServiceEntryVO::ServiceEntryVO( ServiceEntryVO&&  rhs) = default;
ServiceEntryVO& ServiceEntryVO::operator = (ServiceEntryVO&&) = default;
ServiceEntryVO::ServiceEntryVO( const ServiceEntryVO& rhs) = default;
ServiceEntryVO& ServiceEntryVO::operator = ( const ServiceEntryVO& rhs) = default;

std::string ServiceEntryVO::getParentService() const
{
   return pimpl->parentService;
}
std::string ServiceEntryVO::getService() const
{
   return pimpl->service;
}
sf::platform::Uuid ServiceEntryVO::getCallId() const
{
   return pimpl->callId;
}
sf::platform::time_point ServiceEntryVO::getStart() const
{
   return pimpl->start;
}
sf::platform::time_point ServiceEntryVO::getEnd() const
{
   return pimpl->end;
}


void ServiceEntryVO::setParentService( std::string value)
{
   pimpl->parentService = value;
}
void ServiceEntryVO::setService( std::string value)
{
   pimpl->service = value;
}
void ServiceEntryVO::setCallId( sf::platform::Uuid value)
{
   pimpl->callId = value;
}
void ServiceEntryVO::setStart( sf::platform::time_point value)
{
   pimpl->start = value;
}
void ServiceEntryVO::setEnd( sf::platform::time_point value)
{
   pimpl->end = value;
}






void ServiceEntryVO::serialize( casual::sf::archive::Reader& archive)
{
    pimpl->serialize( archive);
}
  
void ServiceEntryVO::serialize( casual::sf::archive::Writer& archive) const
{
    pimpl->serialize( archive);
}



} // monitor
} // traffic
} // casual

	
