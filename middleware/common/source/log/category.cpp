//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/log/category.h"

namespace casual
{
   namespace common::log::category
   {

      Stream parameter{ "parameter"};
      Stream information{ "information"};
      Stream warning{ "warning"};

      // Always on
      Stream error{ "error"};

      namespace verbose
      {
         Stream error{ "error.verbose"};

      } // verbose

      Stream transaction{ "casual.transaction"};
      Stream buffer{ "casual.buffer"};

   } // common::log::category
} // casual

