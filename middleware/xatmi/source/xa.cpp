//!
//! casual
//!

#include "xa.h"
#include "common/transaction/context.h"

#include "common/exception/xa.h"


extern "C"
{
   int ax_reg( int rmid, XID* xid, long flags)
   {
      try
      {
         return casual::common::transaction::Context::instance().resourceRegistration( rmid, xid, flags);
      }
      catch( ...)
      {
         return casual::common::exception::xa::handle();
      }
   }

   int ax_unreg( int rmid, long flags)
   {
      try
      {
         return casual::common::transaction::Context::instance().resourceUnregistration( rmid, flags);
      }
      catch( ...)
      {
         return casual::common::exception::xa::handle();
      }
   }
}



