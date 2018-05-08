//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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

            //!
            //! throws a suitable type based on the code. 
            //! @note no-op if code is tx::ok
            //! @{
            void handle( code::tx code, const std::string& information);
            void handle( code::tx code);
            //! @}

            code::tx handle();

         } // tx 
      } // exception 
   } // common
} // casual

