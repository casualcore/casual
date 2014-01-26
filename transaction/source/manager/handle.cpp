//!
//! handle.cpp
//!
//! Created on: Nov 1, 2013
//!     Author: Lazan
//!

#include "transaction/manager/handle.h"


#include "common/algorithm.h"


namespace casual
{

   namespace transaction
   {
      namespace handle
      {

         void Involved::dispatch( message_type& message)
         {
            auto transcation = common::range::find_if( common::range::make( m_state.transactions), find::Transaction( message.xid));


            if( ! transcation.empty())
            {
               common::range::copy(
                  common::range::make( message.resources),
                  std::back_inserter( transcation.first->resources));

               common::range::trim( transcation.first->resources, common::range::unique( common::range::sort( common::range::make( transcation.first->resources))));
            }
            else
            {
               common::log::error << "resource " << common::range::make( message.resources) << " (process " << message.id << ") claims to be involved in transaction " << message.xid.stringGlobal() << ", which is not known to TM - action: discard" << std::endl;
            }
         }


      } // handle
   } // transaction



} // casual
