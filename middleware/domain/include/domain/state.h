//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_

#include "common/message/type.h"
#include "common/message/pending.h"
#include "common/uuid.h"
#include "common/platform.h"
#include "common/process.h"


#include <unordered_map>
#include <vector>

namespace casual
{
   namespace domain
   {
      namespace state
      {



      } // state


      struct State
      {


         using processes_type = std::unordered_map< common::platform::pid::type, common::process::Handle>;


         processes_type processes;

         std::map< common::Uuid, common::process::Handle> singeltons;

         struct
         {
            std::vector< common::message::pending::Message> replies;
            std::vector< common::message::process::lookup::Request> lookup;
         } pending;


      };


   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_STATE_H_
