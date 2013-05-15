
		

#include <sf/archive.h>


//## includes protected section begin [200.20]

#include "monitor/monitor_vo.h"

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
	 sf::Uuid callId;
	 sf::time_type start;
	 sf::time_type end;
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
std::string MonitorVO::getService() const
{
   return pimpl->service;
}
sf::Uuid MonitorVO::getCallId() const
{
   return pimpl->callId;
}
sf::time_type MonitorVO::getStart() const
{
   return pimpl->start;
}
sf::time_type MonitorVO::getEnd() const
{
   return pimpl->end;
}

void MonitorVO::setParentService( const std::string& value)
{
   pimpl->parentService = value;
}
void MonitorVO::setService( const std::string& value)
{
   pimpl->service = value;
}
void MonitorVO::setCallId( const sf::Uuid& value)
{
   pimpl->callId = value;
}
void MonitorVO::setStart( const sf::time_type& value)
{
   pimpl->start = value;
}
void MonitorVO::setEnd( const sf::time_type& value)
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




}
}
}
}
