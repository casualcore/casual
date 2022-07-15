//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "queue/common/ipc/message.h"
#include "common/strong/type.h"

#include <string>

namespace casual
{
   namespace queue
   {
      namespace remote::queue
      {
         constexpr auto id = common::strong::queue::id{ -1L};
      } // remote::queue

      struct Lookup
      {
         using Semantic = ipc::message::lookup::request::context::Semantic;
         explicit Lookup( std::string queue, Semantic semantic = Semantic::direct);

         ipc::message::lookup::Reply operator () () const;

         const std::string& name() const;

      private:
         std::string m_name;
         common::strong::correlation::id m_correlation;

      };

   } // queue
} // casual


