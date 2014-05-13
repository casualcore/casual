
		

#include <sf/archive.h>


//## includes protected section begin [200.20]

#include "transaction/manager/admin/vo/transaction.h"

//## includes protected section end   [200.20]

namespace vo
{



struct Transaction::Implementation
{
    Implementation()
   //## initialization list protected section begin [200.40]
         : state( 0)
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
      archive & CASUAL_MAKE_NVP( xid);
      archive & CASUAL_MAKE_NVP( state);
      //## additional serialization protected section begin [200.impl.serial.20]
      //## additional serialization protected section end   [200.impl.serial.20]
   }

   //## additional attributes protected section begin [200.impl.attr.10]
   //## additional attributes protected section end   [200.impl.attr.10]
   std::string xid;
   long state;
   //## additional attributes protected section begin [200.impl.attr.20]
   //## additional attributes protected section end   [200.impl.attr.20]

};




Transaction::Transaction()
   : pimpl( new Implementation())  
{
   //## base class protected section begin [200.ctor.10]
   //## base class protected section end   [200.ctor.10]  
}

Transaction::~Transaction() = default;

Transaction::Transaction( Transaction&&  rhs) = default;

Transaction& Transaction::operator = (Transaction&&) = default;


Transaction::Transaction( const Transaction& rhs)
   : pimpl( new Implementation( *rhs.pimpl))
{

}

Transaction& Transaction::operator = ( const Transaction& rhs)
{
    *pimpl = *rhs.pimpl;
    return *this;
}

std::string Transaction::getXid() const
{
   return pimpl->xid;
}
long Transaction::getState() const
{
   return pimpl->state;
}


void Transaction::setXid( std::string value)
{
   pimpl->xid = value;
}
void Transaction::setState( long value)
{
   pimpl->state = value;
}






void Transaction::serialize( casual::sf::archive::Reader& archive)
{
    pimpl->serialize( archive);
}
  
void Transaction::serialize( casual::sf::archive::Writer& archive) const
{
    pimpl->serialize( archive);
}




} // vo

	
