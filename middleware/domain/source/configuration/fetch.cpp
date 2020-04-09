//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/configuration/fetch.h"
#include "domain/common.h"

#include "common/environment/normalize.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"

#include "configuration/message.h"


namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace configuration
      {
         casual::configuration::Model fetch()
         {
            Trace trace{ "configuration::model::fetch"};

            return environment::normalize( communication::ipc::call( 
               communication::instance::outbound::domain::manager::device(),
               casual::configuration::message::Request{ process::handle()})).model;

         }

      } // configuration
   } // domain

} // casual
