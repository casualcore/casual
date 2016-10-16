//!
//! casual
//!

#ifndef CAUSUAL_QUEUE_COMMON_QUEUE_H_
#define CAUSUAL_QUEUE_COMMON_QUEUE_H_


#include "common/message/queue.h"

#include <string>

namespace casual
{
   namespace queue
   {
      struct Lookup
      {
         Lookup( const std::string& queue);

         common::message::queue::lookup::Reply operator () () const;

      private:
         common::Uuid m_correlation;

      };

   } // queue
} // casual

#endif // CAUSUAL_QUEUE_COMMON_QUEUE_H_
