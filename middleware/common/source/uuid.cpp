//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/uuid.h"
#include "common/transcode.h"
#include "common/memory.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <cassert>
#include <ostream>


namespace casual
{

   namespace common
   {

      namespace uuid
      {
         std::string string( const platform::uuid::type& uuid)
         {
            return transcode::hex::encode( uuid);
         }

         std::string string( const Uuid& uuid)
         {
            return string( uuid.get());
         }

         Uuid make()
         {
            Uuid result;
            uuid_generate( result.get());
            return result;
         }

         const Uuid& empty()
         {
            static const Uuid empty;
            return empty;
         }

         bool empty( const Uuid& uuid)
         {
            return uuid == uuid::empty();
         }

      } // uuid

      Uuid::Uuid( const uuid_type& uuid)
      {
         uuid_copy( m_uuid, uuid);
      }

      Uuid::Uuid( std::string_view string)
      {
         transcode::hex::decode( string, m_uuid);
      }

      void Uuid::copy( uuid_type& uuid) const
      {
         uuid_copy( uuid, m_uuid);
      }

      bool Uuid::empty() const
      {
         return  *this == uuid::empty();
      }

      Uuid::operator bool() const noexcept
      {
         return ! empty();
      }

      std::ostream& operator << ( std::ostream& out, const Uuid& uuid)
      {
         if( uuid.empty())
            return out << "nil";
         return transcode::hex::encode( out, uuid.m_uuid);
      }

      std::istream& operator >> ( std::istream& in, Uuid& value)
      {
         std::string string;
         in >> string;

         if( ! std::regex_match( string, std::regex{ "[a-f0-9]{32}"}))
            code::raise::error( code::casual::invalid_argument, "invalid uuid: ", string);

         transcode::hex::decode( string, value.m_uuid);
         return in;
      }

   } // common
} // casual

casual::common::Uuid operator"" _uuid ( const char* data)
{
   return casual::common::Uuid{ std::string_view( data + 2)};
}




