//!
//! lookup.h
//!
//! Created on: Jun 26, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_CALL_LOOKUP_H_
#define CASUAL_COMMON_CALL_LOOKUP_H_

#include "common/uuid.h"
#include "common/message/service.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace call
      {
         namespace service
         {
            struct Lookup
            {
               Lookup( const std::string& service, long flags);
               ~Lookup();
               message::service::lookup::Reply operator () () const;
            private:
               mutable Uuid m_correlation;
            };

         } // service

      } // call
   } // common

} // casual

#endif // LOOKUP_H_
