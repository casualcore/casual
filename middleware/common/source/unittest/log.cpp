//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest/log.h"

#include "common/signal.h"
#include "common/execution.h"
#include "common/communication/ipc.h"

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
               execution::reset();
               signal::clear();

               communication::ipc::inbound::device().clear();
            }

            Scope::~Scope() { signal::clear();}

         } // clean
      } // unittest
   } // common
} // casual
