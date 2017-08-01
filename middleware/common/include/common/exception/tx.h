//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_TX_H_
#define CASUAL_COMMON_EXCEPTION_TX_H_

#include "common/error/code/tx.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace tx 
         {

            using exception = common::exception::base_error< error::code::tx>;

            template< error::code::tx error>
            using base = common::exception::basic_error< exception, error>;
            

            using Fail = base< error::code::tx::fail>;

            using Error =  base< error::code::tx::error>;

            using Protocol =  base< error::code::tx::protocol>;

            using Argument =  base< error::code::tx::argument>;

            using Outside =  base< error::code::tx::outside>;

            namespace no
            {
               using Begin = base< error::code::tx::no_begin>;

               using Support = base< error::code::tx::not_supported>;
            } // no
         } // tx 
      } // exception 
   } // common
} // casual



#endif