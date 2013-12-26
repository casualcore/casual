
		

#include <sf/archive.h>


//## includes protected section begin [200.20]

#include "monitor/monitorvo.h"

//## includes protected section end   [200.20]

namespace casual
{
namespace statistics
{
namespace monitor
{
namespace vo
{



struct MonitorVO::Implementation
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
      archive & CASUAL_MAKE_NVP( srv);
      archive & CASUAL_MAKE_NVP( callId);
      archive & CASUAL_MAKE_NVP( start);
      archive & CASUAL_MAKE_NVP( end);
      //## additional serialization protected section begin [200.impl.serial.20]
      //## additional serialization protected section end   [200.impl.serial.20]
   }

   //## additional attributes protected section begin [200.impl.attr.10]
   //## additional attributes protected section end   [200.impl.attr.10]
   std::string parentService;
   std::string srv;
   sf::platform::Uuid callId;
   sf::platform::time_type start;
   sf::platform::time_type end;
   //## additional attributes protected section begin [200.impl.attr.20]
   //## additional attributes protected section end   [200.impl.attr.20]

};




MonitorVO::MonitorVO()
   : pimpl( new Implementation())  
{
   //## base class protected section begin [200.ctor.10]
   //## base class protected section end   [200.ctor.10]  
}

MonitorVO::~MonitorVO() = default;

MonitorVO::MonitorVO( MonitorVO&&  rhs) = default;

MonitorVO& MonitorVO::operator = (MonitorVO&&) = default;


MonitorVO::MonitorVO( const MonitorVO& rhs)
   : pimpl( new Implementation( *rhs.pimpl))
{

}

MonitorVO& MonitorVO::operator = ( const MonitorVO& rhs)
{
    *pimpl = *rhs.pimpl;
    return *this;
}

std::string MonitorVO::getParentService() const
{
   return pimpl->parentService;
}
std::string MonitorVO::getSrv() const
{
   return pimpl->srv;
}
sf::platform::Uuid MonitorVO::getCallId() const
{
   return pimpl->callId;
}
sf::platform::time_type MonitorVO::getStart() const
{
   return pimpl->start;
}
sf::platform::time_type MonitorVO::getEnd() const
{
   return pimpl->end;
}


void MonitorVO::setParentService( std::string value)
{
   pimpl->parentService = value;
}
void MonitorVO::setSrv( std::string value)
{
   pimpl->srv = value;
}
void MonitorVO::setCallId( sf::platform::Uuid value)
{
   pimpl->callId = value;
}
void MonitorVO::setStart( sf::platform::time_type value)
{
   pimpl->start = value;
}
void MonitorVO::setEnd( sf::platform::time_type value)
{
   pimpl->end = value;
}






void MonitorVO::serialize( casual::sf::archive::Reader& archive)
{
    pimpl->serialize( archive);
}
  
void MonitorVO::serialize( casual::sf::archive::Writer& archive) const
{
    pimpl->serialize( archive);
}




} // vo
} // monitor
} // statistics
} // casual

	
