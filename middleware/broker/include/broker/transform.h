//!
//! transform.h
//!
//! Created on: Sep 13, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_BROKERTRANSFORM_H_
#define CASUAL_BROKERTRANSFORM_H_

#include "broker/state.h"


#include "config/domain.h"

#include "common/message/server.h"
#include "common/message/transaction.h"


namespace casual
{
   namespace broker
   {
      namespace transform
      {
         namespace configuration
         {

            struct Service
            {
               state::Service operator () ( const config::domain::Service& service) const;
            };


         } // configuration



         struct Service
         {
            state::Service operator () ( const common::message::Service& value) const;
         };

         struct Instance
         {
            //state::Instance operator () ( const common::message::server::connect::Request& message) const;

            common::process::Handle operator () ( const state::Instance& value) const;

         };



      } // transform
   } // broker
} // casual

#endif // TRANSFORM_H_
