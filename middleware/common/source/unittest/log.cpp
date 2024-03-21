//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest/log.h"

#include "common/unittest/environment.h"

#include "common/signal.h"
#include "common/execution.h"
#include "common/communication/ipc.h"
#include "common/environment.h"
#include "common/domain.h"

namespace casual
{
   namespace common::unittest
   {

      common::log::Stream log{ "casual.unittest"};
      common::log::Stream trace{ "casual.unittest.trace"};

      namespace clean
      {
         Scope::Scope() 
         { 
            // set that we're in _unittest-context_
            common::environment::variable::set( common::environment::variable::name::unittest::context, "");
            
            execution::reset();
            signal::clear();

            communication::ipc::inbound::device().clear();
         }

         Scope::~Scope() 
         { 
            signal::clear();
            common::environment::variable::unset( common::environment::variable::name::unittest::context);
            
            domain::identity( {}); 
         }
      } // clean

   } // common::unittest
} // casual
