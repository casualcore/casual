//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "transaction/manager/state.h"

#include "transaction/manager/admin/transactionvo.h"


#include <vector>

namespace casual
{
   namespace transaction
   {
      namespace manager
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

               std::vector< admin::resource::Proxy> insances( State& state, std::vector< admin::update::Instances> instances);


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
      } // manager
   } // transaction
} // casual


