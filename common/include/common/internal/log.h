//!
//! log_internal.h
//!
//! Created on: Dec 30, 2013
//!     Author: Lazan
//!

#ifndef INTERNAL_LOG_H_
#define INTERNAL_LOG_H_

#include "common/log.h"

namespace casual
{
   namespace common
   {
      namespace log
      {
         namespace internal
         {

            //!
            //! Log with category 'casual.debug'
            //!
            extern std::ostream debug;

            //!
            //! Log with category 'casual.trace'
            //!
            extern std::ostream trace;

            //!
            //! Log with category 'casual.transaction'
            //!
            extern std::ostream transaction;


         } // internal
      } // log
   } // common


} // casual

#endif // INTERNAL_LOG_H_
