//!
//! transform.cpp
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!


#include "broker/transform.h"

#include "common/environment.h"
#include "common/chronology.h"

namespace casual
{

   namespace broker
   {
      namespace transform
      {

         namespace configuration
         {


            state::Service Service::operator () ( const config::domain::Service& service) const
            {
               state::Service result;

               result.information.name = service.name;
               result.information.timeout = common::chronology::from::string( service.timeout);

               return result;
            }

         } // configuration



         state::Service Service::operator () ( const common::message::Service& value) const
         {
            state::Service result;

            result.information.name = value.name;
            //result.information.timeout = value.timeout;
            result.information.transaction = value.transaction;
            result.information.type = value.type;

            // TODD: set against configuration


            return result;
         }

         /*
         state::Instance Instance::operator () ( const common::message::server::connect::Request& message) const
         {
            state::Instance result;

            //result->path = message.path;
            result.process = message.process;

            return result;
         }
         */


         common::process::Handle Instance::operator () ( const state::Instance& value) const
         {
            return value.process;
         }

      } // transform
   } // broker

} // casual


