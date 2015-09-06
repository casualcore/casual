//!
//! manager_action.h
//!
//! Created on: Aug 14, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_ACTION_H_
#define MANAGER_ACTION_H_

#include "transaction/manager/state.h"



namespace casual
{
   namespace transaction
   {
      namespace action
      {

         void configure( State& state);


         namespace boot
         {
            struct Proxie : state::Base
            {
               using state::Base::Base;

               void operator () ( state::resource::Proxy& proxy);
            };
         } // boot



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
