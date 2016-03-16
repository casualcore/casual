//!
//! platform.h
//!
//! Created on: Dec 23, 2013
//!     Author: Lazan
//!

#ifndef PLATFORM_H_
#define PLATFORM_H_


#include "common/uuid.h"
#include "common/platform.h"
#include "common/transaction/id.h"
#include "common/algorithm.h"



#include "sf/namevaluepair.h"


#include <chrono>

namespace casual
{
   namespace common
   {
      namespace process
      {
         struct Handle;
      } // process

      namespace domain
      {
         struct Identity;
      } // domain

   } // common
   namespace sf
   {

      namespace platform
      {
         typedef common::platform::raw_buffer_type raw_buffer_type;
         using const_raw_buffer_type = common::platform::const_raw_buffer_type;


         using binary_type = common::platform::binary_type;

         using const_binary_range_type = common::Range< binary_type::const_iterator>;

         using Uuid = common::Uuid;

         namespace uuid
         {
            using namespace common::uuid;
         } // uuid


         using time_point = common::platform::time_point;

         using pid_type = common::platform::pid_type;

         using queue_id_type = common::platform::queue_id_type;

      } // platform

      namespace archive
      {
         class Reader;
         class Writer;

         void serialize( Reader& archive, platform::Uuid& value, const char* name);
         void serialize( Writer& archive, const platform::Uuid& value, const char* name);

         void serialize( Reader& archive, common::process::Handle& value, const char* name);
         void serialize( Writer& archive, const common::process::Handle& value, const char* name);

         void serialize( Reader& archive, common::domain::Identity& value, const char* name);
         void serialize( Writer& archive, const common::domain::Identity& value, const char* name);


         void serialize( Reader& archive, platform::time_point& value, const char* name);
         void serialize( Writer& archive, const platform::time_point& value, const char* name);

         void serialize( Reader& archive, std::chrono::nanoseconds& value, const char* name);
         void serialize( Writer& archive, const std::chrono::nanoseconds& value, const char* name);

         template< typename R, typename P>
         void serialize( Reader& archive, std::chrono::duration< R, P>& value, const char* name)
         {
            std::chrono::nanoseconds ns;
            serialize( archive, ns, name);
            value = std::chrono::duration_cast< std::chrono::duration< R, P>>( ns);
         }

         template< typename R, typename P>
         void serialize( Writer& archive, const std::chrono::duration< R, P>& value, const char* name)
         {
            serialize( archive, std::chrono::duration_cast<std::chrono::nanoseconds>( value), name);
         }

      } // archive
   } // sf

} // casual

#endif // PLATFORM_H_
