//!
//! platform
//!
//! Created on: Dec 23, 2013
//!     Author: Lazan
//!

#include "sf/platform.h"

#include "sf/archive/archive.h"


namespace casual
{
   namespace sf
   {
      namespace archive
      {

         void serialize( Reader& archive, platform::Uuid& value, const char* name)
         {
            std::string uuid;
            archive >> sf::makeNameValuePair( name, uuid);

            value = platform::Uuid{ uuid};
         }

         void serialize( Writer& archive, const platform::Uuid& value, const char* name)
         {
            archive << sf::makeNameValuePair( name, common::uuid::string( value));
         }


         void serialize( Reader& archive, platform::time_point& value, const char* name)
         {
            platform::time_point::rep representation;
            archive >> sf::makeNameValuePair( name, representation);
            value = platform::time_point( platform::time_type::duration( representation));
         }

         void serialize( Writer& archive, const platform::time_point& value, const char* name)
         {
            archive << sf::makeNameValuePair( name, value.time_since_epoch().count());
         }

      } // archive

   } // sf

} // casual
