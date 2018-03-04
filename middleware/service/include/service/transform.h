//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_BROKERTRANSFORM_H_
#define CASUAL_BROKERTRANSFORM_H_

#include "configuration/domain.h"
#include "service/manager/state.h"
#include "service/manager/admin/managervo.h"

#include "common/message/server.h"
#include "common/message/transaction.h"




namespace casual
{
   namespace service
   {
      namespace transform
      {


         struct Service
         {
            manager::state::Service operator () ( const common::message::Service& value) const;
         };

         struct Instance
         {
            //state::Instance operator () ( const common::message::server::connect::Request& message) const;

            common::process::Handle operator () ( const manager::state::instance::Local& value) const;


         };


         manager::admin::StateVO state( const manager::State& state);


      } // transform
   } // service
} // casual

#endif // TRANSFORM_H_
