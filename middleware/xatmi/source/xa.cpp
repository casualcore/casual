//!
//! casual
//!

#include "xa.h"
#include "common/transaction/context.h"

#include "common/exception/xa.h"


extern "C"
{
   int ax_reg( int rmid, XID* xid, long /* flags reserved for future use */)
   {
      try
      {
         casual::common::transaction::context().resource_registration( rmid, xid);
         return casual::common::cast::underlying( casual::common::error::code::ax::ok);
      }
      catch( ...)
      {
         return casual::common::exception::ax::handle();
      }
   }

   int ax_unreg( int rmid, long /* flags reserved for future use */)
   {
      try
      {
         casual::common::transaction::context().resource_unregistration( rmid);
         return casual::common::cast::underlying( casual::common::error::code::ax::ok);
      }
      catch( ...)
      {
         return casual::common::exception::ax::handle();
      }
   }
}



