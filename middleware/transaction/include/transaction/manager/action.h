//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef MANAGER_ACTION_H_
#define MANAGER_ACTION_H_

#include "transaction/manager/state.h"

#include "transaction/manager/admin/transactionvo.h"


#include <vector>

namespace casual
{
   namespace transaction
   {
      namespace action
      {

         void configure( State& state);


         namespace resource
         {
            struct Instances : state::Base
            {
               using state::Base::Base;

               void operator () ( state::resource::Proxy& proxy);
            };

            std::vector< vo::resource::Proxy> insances( State& state, std::vector< vo::update::Instances> instances);


            bool request( State& state, state::pending::Request& message);

         } // resource



         namespace persistent
         {
            struct Send : state::Base
            {
               using state::Base::Base;

               bool operator () ( state::pending::Reply& message) const;

               bool operator () ( state::pending::Request& message) const;

            };

         } // pending


      } // action
   } // transaction
} // casual

#endif // MANAGER_ACTION_H_
