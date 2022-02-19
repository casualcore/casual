//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/unittest/discover.h"
#include "domain/common.h"
#include "domain/discovery/api.h"

#include "common/communication/ipc.h"

namespace casual
{
   using namespace common;
   namespace domain::unittest
   {
      void discover( std::vector< std::string> services, std::vector< std::string> queues)
      {
         Trace trace{ "domain::unittest::discover"};
         if( auto correlation = casual::domain::discovery::request( std::move( services), std::move( queues)))
            communication::ipc::receive< message::discovery::api::Reply>( correlation);
      }

      
   } // domain::unittest
   
} // casual