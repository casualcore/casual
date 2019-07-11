//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
         casual::common::transaction::context().resource_registration( casual::common::strong::resource::id{ rmid}, xid);
         return casual::common::cast::underlying( casual::common::code::ax::ok);
      }
      catch( ...)
      {
         return casual::common::cast::underlying( casual::common::exception::ax::handle());
      }
   }

   int ax_unreg( int rmid, long /* flags reserved for future use */)
   {
      try
      {
         casual::common::transaction::context().resource_unregistration( casual::common::strong::resource::id{ rmid});
         return casual::common::cast::underlying( casual::common::code::ax::ok);
      }
      catch( ...)
      {
         return casual::common::cast::underlying( casual::common::exception::ax::handle());
      }
   }
}



