//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/message/protocol.h"

#include "common/algorithm/is.h"
#include "common/environment.h"

namespace casual
{
   using namespace common;

   namespace gateway::message::protocol
   {
      // make sure the versions are sorted in reverse order.
      static_assert( algorithm::is::sorted( range::reverse( protocol::versions)));


      Version version()
      {
         auto transform_version = []( auto value)
         {
            Version version{ value};

            if( ! algorithm::contains( protocol::versions, version))
               code::raise::error( code::casual::invalid_configuration, "invalid version value: ", value);

            return version;
         };

         static const auto value = environment::variable::get< platform::size::type>( environment::variable::name::internal::gateway::protocol::version)
            .transform( transform_version)
            .value_or( Version::current);

         return value;
      }

   } //gateway::message::protocol
} // casual