//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "transaction/common.h"


namespace casual
{
   namespace transaction
   {
      common::log::Stream& log = common::log::category::transaction;

      namespace trace
      {
         common::log::Stream log{ "casual.transaction.trace"};
      } // trace

      namespace verbose
      {
         common::log::Stream log{ "casual.transaction.verbose"};
      } // verbose

   } // transaction

} // casual
