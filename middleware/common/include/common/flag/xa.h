//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/xa/flag.h"

#include "common/flag.h"

namespace casual 
{
   namespace common 
   {
      namespace flag
      {
         namespace xa 
         {
            //!
            //! Flag definition for the RM switch
            //!
            namespace resource 
            {
               enum class Flag : long
               {  
                  no_flags = TMNOFLAGS,
                  dynamic = TMREGISTER,
                  no_migrate = TMNOMIGRATE,
                  asynchronous = TMUSEASYNC
               };
               using Flags = common::Flags< resource::Flag>;
            } // resource 


            enum class Flag : long
            {
               no_flags = TMNOFLAGS,
               asynchronous = TMASYNC,
               one_phase = TMONEPHASE,
               fail = TMFAIL,
               no_wait = TMNOWAIT,
               resume = TMRESUME,
               success = TMSUCCESS,
               suspend = TMSUSPEND,
               start_scan = TMSTARTRSCAN,
               end_scan = TMENDRSCAN,
               wait_any = TMMULTIPLE,
               join = TMJOIN,
               migrate = TMMIGRATE
            };
            using Flags = common::Flags< xa::Flag>;

         } // xa 
      } // flag
   } // common 
} // casual 

//
// To help prevent missuse of "raw flags"
//

#ifndef CASUAL_NO_XATMI_UNDEFINE

#undef TMNOFLAGS
#undef TMREGISTER
#undef TMNOMIGRATE
#undef TMUSEASYNC
#undef TMNOFLAGS
#undef TMASYNC
#undef TMONEPHASE
#undef TMFAIL
#undef TMNOWAIT
#undef TMRESUME
#undef TMSUCCESS
#undef TMSUSPEND
#undef TMSTARTRSCAN
#undef TMENDRSCAN
#undef TMMULTIPLE
#undef TMJOIN
#undef TMMIGRATE

#endif // CASUAL_NO_XATMI_UNDEFINE

