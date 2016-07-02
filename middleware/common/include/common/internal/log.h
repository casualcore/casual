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
            extern Stream debug;

            //!
            //! Log with category 'casual.trace'
            //!
            extern Stream trace;

            //!
            //! Log with category 'casual.transaction'
            //!
            extern Stream transaction;


            //!
            //! Log with category 'casual.ipc'
            //!
            extern Stream ipc;


            //!
            //! Log with category 'casual.buffer'
            //!
            extern Stream buffer;


         } // internal
      } // log
   } // common


} // casual

#endif // INTERNAL_LOG_H_
