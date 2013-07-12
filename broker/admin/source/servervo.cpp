
		

#include <sf/archive.h>


//## includes protected section begin [300.20]

#include "broker/servervo.h"

//## includes protected section end   [300.20]

namespace casual
{
namespace broker
{
namespace admin
{



struct ServerVO::Implementation
{
    Implementation()
   //## initialization list protected section begin [300.40]
         : pid( 0), queue( 0), idle( false)
   //## initialization list protected section end   [300.40]
   {
      //## ctor protected section begin [300.impl.ctor.10]
      //## ctor protected section end   [300.impl.ctor.10]
   }

 
   template< typename A>
   void serialize( A& archive)
   {
      //## additional serialization protected section begin [300.impl.serial.10]
      //## additional serialization protected section end   [300.impl.serial.10]
      archive & CASUAL_MAKE_NVP( pid);
      archive & CASUAL_MAKE_NVP( path);
      archive & CASUAL_MAKE_NVP( queue);
      archive & CASUAL_MAKE_NVP( idle);
      //## additional serialization protected section begin [300.impl.serial.20]
      //## additional serialization protected section end   [300.impl.serial.20]
   }

   //## additional attributes protected section begin [300.impl.attr.10]
   //## additional attributes protected section end   [300.impl.attr.10]
   long pid;
   std::string path;
   long queue;
   bool idle;
   //## additional attributes protected section begin [300.impl.attr.20]
   //## additional attributes protected section end   [300.impl.attr.20]

};




ServerVO::ServerVO()
   : pimpl( new Implementation())  
{
   //## base class protected section begin [300.ctor.10]
   //## base class protected section end   [300.ctor.10]  
}

ServerVO::~ServerVO() = default;

ServerVO::ServerVO( ServerVO&&  rhs) = default;

ServerVO& ServerVO::operator = (ServerVO&&) = default;


ServerVO::ServerVO( const ServerVO& rhs)
   : pimpl( new Implementation( *rhs.pimpl))
{

}

ServerVO& ServerVO::operator = ( const ServerVO& rhs)
{
    *pimpl = *rhs.pimpl;
    return *this;
}

long ServerVO::getPid() const
{
   return pimpl->pid;
}
std::string ServerVO::getPath() const
{
   return pimpl->path;
}
long ServerVO::getQueue() const
{
   return pimpl->queue;
}
bool ServerVO::getIdle() const
{
   return pimpl->idle;
}


void ServerVO::setPid( long value)
{
   pimpl->pid = value;
}
void ServerVO::setPath( std::string value)
{
   pimpl->path = value;
}
void ServerVO::setQueue( long value)
{
   pimpl->queue = value;
}
void ServerVO::setIdle( bool value)
{
   pimpl->idle = value;
}






void ServerVO::serialize( casual::sf::archive::Reader& archive)
{
    pimpl->serialize( archive);
}
  
void ServerVO::serialize( casual::sf::archive::Writer& archive) const
{
    pimpl->serialize( archive);
}




} // admin
} // broker
} // casual

	
