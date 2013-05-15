//!
//! uuid.h
//!
//! Created on: May 5, 2013
//!     Author: Lazan
//!

#ifndef UUID_H_
#define UUID_H_

#include "common/uuid.h"

#include "sf/archive.h"

namespace casual
{
   namespace sf
   {
      typedef common::Uuid Uuid;


      namespace archive
      {
         inline void serialize( archive::Reader& archive, const char* name, Uuid& value)
         {
            std::string uuid;
            archive >> sf::makeNameValuePair( name, uuid);

            value.string( uuid);
         }

         inline void serialize( archive::Writer& archive, const char* name, const Uuid& value)
         {
            archive << sf::makeNameValuePair( name, value.string());
         }
      }

      typedef common::time_type time_type;

      namespace archive
      {
         inline void serialize( archive::Reader& archive, const char* name, time_type& value)
         {
            time_type::rep representation;
            archive >> sf::makeNameValuePair( name, representation);
            value = common::time_type( common::time_type::duration( representation));
         }

         inline void serialize( archive::Writer& archive, const char* name, const time_type& value)
         {
            archive << sf::makeNameValuePair( name, value.time_since_epoch().count());
         }
      }


   } // sf
} // casual


#endif /* UUID_H_ */
