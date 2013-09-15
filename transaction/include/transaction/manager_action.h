//!
//! manager_action.h
//!
//! Created on: Aug 14, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_ACTION_H_
#define MANAGER_ACTION_H_

#include "transaction/manager_state.h"

namespace casual
{
   namespace transaction
   {
      namespace action
      {

         /*
         template< typename Q>
         struct Send : state::Base
         {
            using state::Base::Base;

            auto operator () ( state::pending::Reply& reply) -> decltype( std::declval< Q>()( reply.reply))
            {
               Q queue( reply.target, m_state);
               return queue( reply.reply);
            }
         };
         */


         struct base_action
         {

         };


      } // action
   } // transaction


} // casual

#endif // MANAGER_ACTION_H_
