//!
//! manager_action.h
//!
//! Created on: Aug 14, 2013
//!     Author: Lazan
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

         void configure( State& state, const std::string& resource_file);


         namespace resource
         {
            struct Instances : state::Base
            {
               using state::Base::Base;

               void operator () ( state::resource::Proxy& proxy);
            };

            std::vector< vo::resource::Proxy> insances( State& state, std::vector< vo::update::Instances> instances);

            namespace instance
            {
               bool request( State& state, const common::communication::message::Complete& message, state::resource::Proxy::Instance& instance);
            } // instance

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
