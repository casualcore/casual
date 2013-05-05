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

      template< typename R>
      archive::Reader& operator >> ( archive::Reader& archive, const NameValuePair< Uuid, R>&& nvp)
      {
         std::string uuid;
         archive >> sf::makeNameValuePair( nvp.getName(), uuid);

         nvp.getValue().string( uuid);

         return archive;
      }

      template< typename R>
      archive::Writer& operator << ( archive::Writer& archive, const NameValuePair< Uuid, R>&& nvp)
      {
         archive << sf::makeNameValuePair( nvp.getName(), nvp.getConstValue().string());

         return archive;
      }
   } // sf
} // casual


#endif /* UUID_H_ */
