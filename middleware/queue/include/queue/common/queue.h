//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



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
      using id = common::value::Optional< platform::size::type, 0, tag::type>;

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


