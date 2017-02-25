//!
//! casual 
//!

#include "common/log/category.h"

namespace casual
{
   namespace common
   {
      namespace log
      {
         namespace category
         {


            Stream parameter{ "parameter"};
            Stream information{ "information"};
            Stream warning{ "warning"};

            //
            // Always on
            //
            Stream error{ "error"};


            Stream transaction{ "casual.transaction"};
            Stream buffer{ "casual.buffer"};

         } // category
      } // log
   } // common

} // casual

