//!
//! casual
//!

#ifndef CASUAL_BROKERTRANSFORM_H_
#define CASUAL_BROKERTRANSFORM_H_

#include "configuration/domain.h"
#include "broker/state.h"
#include "broker/admin/brokervo.h"

#include "common/message/server.h"
#include "common/message/transaction.h"




namespace casual
{
   namespace broker
   {
      namespace transform
      {


         struct Service
         {
            state::Service operator () ( const common::message::Service& value) const;
         };

         struct Instance
         {
            //state::Instance operator () ( const common::message::server::connect::Request& message) const;

            common::process::Handle operator () ( const state::instance::Local& value) const;


         };


         admin::StateVO state( const broker::State& state);


      } // transform
   } // broker
} // casual

#endif // TRANSFORM_H_
