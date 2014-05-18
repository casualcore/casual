//!
//! utility.h
//!
//! Created on: Feb 6, 2014
//!     Author: Lazan
//!

#ifndef UTILITY_H_
#define UTILITY_H_

#include "common/transaction_id.h"

namespace casual
{
   namespace transaction
   {
      namespace scoped
      {
         namespace log
         {
            struct State
            {
               State( const common::transaction::ID& xid)
               {

               }

            };
         } // log
      } // scoped

   } // transaction



} // casual

#endif // UTILITY_H_
