//!
//! xa.cpp
//!
//! Created on: Jul 14, 2013
//!     Author: Lazan
//!

#include "xa.h"
#include "common/transaction/context.h"
#include "common/error.h"


int ax_reg( int rmid, XID* xid, long flags)
{
   try
   {
      return casual::common::transaction::Context::instance().resourceRegistration( rmid, xid, flags);
   }
   catch( ...)
   {
      return casual::common::error::handler();
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
      return casual::common::error::handler();
   }
}



