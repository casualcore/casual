//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_PROCESS_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_PROCESS_H_

#include "common/process.h"

#include <string>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace mockup
      {

         struct Process
         {
            Process( const std::string& executable, const std::vector< std::string>& arguments);

            ~Process();

            common::process::Handle handle() const;

         private:
            common::process::Handle m_process;
         };


      } // mockup
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MOCKUP_PROCESS_H_
