//!
//! platform
//!
//! Created on: Dec 23, 2013
//!     Author: Lazan
//!

#include "sf/platform.h"

#include "sf/archive.h"


namespace casual
{
   namespace sf
   {
      namespace archive
      {

         void serialize( Reader& archive, const char* name, platform::Uuid& value)
         {
            std::string uuid;
            archive >> sf::makeNameValuePair( name, uuid);

            value.string( uuid);
         }

         void serialize( Writer& archive, const char* name, const platform::Uuid& value)
         {
            archive << sf::makeNameValuePair( name, value.string());
         }


         void serialize( Reader& archive, const char* name, platform::time_type& value)
         {
            platform::time_type::rep representation;
            archive >> sf::makeNameValuePair( name, representation);
            value = platform::time_type( platform::time_type::duration( representation));
         }

         void serialize( Writer& archive, const char* name, const platform::time_type& value)
         {
            archive << sf::makeNameValuePair( name, value.time_since_epoch().count());
         }

      } // archive

   } // sf

} // casual
