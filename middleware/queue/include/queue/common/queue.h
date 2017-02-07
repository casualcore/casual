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
         explicit Lookup( std::string queue);

         common::message::queue::lookup::Reply operator () () const;

         const std::string& name() const;

      private:
         std::string m_name;
         common::Uuid m_correlation;

      };

   } // queue
} // casual

#endif // CAUSUAL_QUEUE_COMMON_QUEUE_H_
