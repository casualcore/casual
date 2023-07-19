//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xa.h"
#include "common/transaction/context.h"

#include "common/code/xa.h"
#include "common/code/category.h"
#include "common/exception/capture.h"

namespace local
{
   namespace
   {
      int handle()
      {
         auto error = casual::common::exception::capture();

         if( casual::common::code::is::category< casual::common::code::ax>( error.code()))
               return error.code().value();

         return std::to_underlying( casual::common::code::ax::error);
      }
   } // <unnamed>
} // local


extern "C"
{
   int ax_reg( int rmid, XID* xid, long /* flags reserved for future use */)
   {
      try
      {
         return std::to_underlying( 
            casual::common::transaction::context().resource_registration( casual::common::strong::resource::id{ rmid}, xid));
      }
      catch( ...)
      {
         return local::handle();
      }
   }

   int ax_unreg( int rmid, long /* flags reserved for future use */)
   {
      try
      {
         return std::to_underlying( 
            casual::common::transaction::context().resource_unregistration( casual::common::strong::resource::id{ rmid}));
      }
      catch( ...)
      {
         return local::handle();
      }
   }
}



