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
      using common::Uuid;

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

   } // sf
} // casual


#endif /* UUID_H_ */
