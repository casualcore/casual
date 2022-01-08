//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest/log.h"

#include "common/signal.h"
#include "common/execution.h"
#include "common/communication/ipc.h"
#include "common/environment.h"

namespace casual
{
   namespace common
   {
      namespace unittest
      {
         common::log::Stream log{ "casual.unittest"};
         common::log::Stream trace{ "casual.unittest.trace"};

         namespace clean
         {
            Scope::Scope() 
            { 
               // set that we're in _unittest-context_
               environment::variable::set( environment::variable::name::unittest::context, "");
               
               execution::reset();
               signal::clear();

               communication::ipc::inbound::device().clear();
            }

            Scope::~Scope() 
            { 
               signal::clear();
               environment::variable::unset( environment::variable::name::unittest::context);
            }

         } // clean
      } // unittest
   } // common
} // casual
