//!
//! platform
//!
//! Created on: Dec 23, 2013
//!     Author: Lazan
//!

#include "sf/platform.h"

#include "sf/archive/archive.h"


#include "common/process.h"
#include "common/domain.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {

         void serialize( Reader& archive, platform::Uuid& value, const char* name)
         {
            platform::binary_type uuid;
            archive >> sf::makeNameValuePair( name, uuid);

            common::range::copy_max( uuid, value.get());
         }

         void serialize( Writer& archive, const platform::Uuid& value, const char* name)
         {
            platform::binary_type uuid( sizeof( value.get()));
            common::range::copy( value.get(), std::begin( uuid));

            archive << sf::makeNameValuePair( name, uuid);
         }

         void serialize( Reader& archive, common::process::Handle& value, const char* name)
         {
            if( archive.serialtype_start( name))
            {
               archive >> sf::makeNameValuePair( "pid", value.pid);
               archive >> sf::makeNameValuePair( "queue", value.queue);
            }
            archive.serialtype_end( name);
         }
         void serialize( Writer& archive, const common::process::Handle& value, const char* name)
         {
            if( archive.serialtype_start( name))
            {
               archive << sf::makeNameValuePair( "pid", value.pid);
               archive << sf::makeNameValuePair( "queue", value.queue);
            }
            archive.serialtype_end( name);
         }

         void serialize( Reader& archive, common::domain::Identity& value, const char* name)
         {
            if( archive.serialtype_start( name))
            {
               archive >> sf::makeNameValuePair( "name", value.name);
               archive >> sf::makeNameValuePair( "id", value.id);
            }
            archive.serialtype_end( name);

         }

         void serialize( Writer& archive, const common::domain::Identity& value, const char* name)
         {
            if( archive.serialtype_start( name))
            {
               archive << sf::makeNameValuePair( "name", value.name);
               archive << sf::makeNameValuePair( "id", value.id);
            }
            archive.serialtype_end( name);
         }


         void serialize( Reader& archive, platform::time_point& value, const char* name)
         {
            platform::time_point::rep representation;
            archive >> sf::makeNameValuePair( name, representation);
            value = platform::time_point( platform::time_point::duration( representation));
         }

         void serialize( Writer& archive, const platform::time_point& value, const char* name)
         {
            archive << sf::makeNameValuePair( name, value.time_since_epoch().count());
         }

         void serialize( Reader& archive, std::chrono::nanoseconds& value, const char* name)
         {
            decltype( value.count()) count;
            archive >> sf::makeNameValuePair( name, count);
            value = std::chrono::nanoseconds( count);
         }

         void serialize( Writer& archive, const std::chrono::nanoseconds& value, const char* name)
         {
            archive << sf::makeNameValuePair( name, value.count());
         }

      } // archive

   } // sf

} // casual
