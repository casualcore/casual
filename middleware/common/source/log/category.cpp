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

      namespace event
      {
         Stream service{ "casual.event.service"};
         Stream server{ "casual.event.server"};
         Stream transaction{ "casual.event.transaction"};

         namespace message
         {
            namespace part
            {
               Stream sent{ "casual.event.message.part.sent"};
               Stream received{ "casual.event.message.part.received"};
            } // part

            Stream sent{ "casual.event.message.sent"};
            Stream received{ "casual.event.message.received"};

         } // message

      } // event

   } // common::log::category
} // casual

