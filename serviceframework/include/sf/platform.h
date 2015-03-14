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


#include "sf/namevaluepair.h"

namespace casual
{
   namespace sf
   {

      namespace platform
      {
         typedef common::platform::raw_buffer_type raw_buffer_type;
         using const_raw_buffer_type = common::platform::const_raw_buffer_type;

         typedef common::platform::binary_type binary_type;

         typedef common::Uuid Uuid;

         namespace uuid
         {
            using namespace common::uuid;
         } // uuid


         typedef common::platform::time_point time_type;
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


         void serialize( Reader& archive, platform::time_point& value, const char* name);

         void serialize( Writer& archive, const platform::time_point& value, const char* name);

      } // archive
   } // sf

} // casual

#endif // PLATFORM_H_
