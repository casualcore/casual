//!
//! casual
//!

#ifndef CASUAL_GATEWAY_MANAGER_ADMIN_VO_H
#define CASUAL_GATEWAY_MANAGER_ADMIN_VO_H


#include "sf/namevaluepair.h"
#include "sf/platform.h"

#include "common/domain.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace admin
         {
            namespace vo
            {
               inline namespace v1_0
               {

                  struct State
                  {
                     long processes;
                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( processes);
                     })

                  };


               } // v1_0
            } // vo
         } // admin
      } // manager
   } // domain
} // casual

#endif // BROKERVO_H_
