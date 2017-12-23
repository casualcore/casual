//!
//! casual
//!

#ifndef CASUAL_QUEUE_COMMON_QUEUE_H_
#define CASUAL_QUEUE_COMMON_QUEUE_H_


#include "common/message/queue.h"
#include "common/value/optional.h"

#include <string>

namespace casual
{
   namespace queue
   {
      namespace tag
      {
         struct type{};
      } // tag
      using id = common::value::Optional< common::platform::size::type, 0, tag::type>;

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

#endif // CASUAL_QUEUE_COMMON_QUEUE_H_
