//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_TX_H_
#define CASUAL_COMMON_EXCEPTION_TX_H_

#include "common/code/tx.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace tx 
         {

            using exception = common::exception::base_error< code::tx>;

            template< code::tx error>
            using base = common::exception::basic_error< exception, error>;
            

            using Fail = base< code::tx::fail>;

            using Error =  base< code::tx::error>;

            using Protocol =  base< code::tx::protocol>;

            using Argument =  base< code::tx::argument>;

            using Outside =  base< code::tx::outside>;

            namespace no
            {
               using Begin = base< code::tx::no_begin>;

               using Support = base< code::tx::not_supported>;
            } // no
         } // tx 
      } // exception 
   } // common
} // casual



#endif
