//!
//! casual_utility_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "common/uuid.h"

#include <cassert>
#include <ostream>


namespace casual
{

	namespace common
	{

      namespace uuid
      {
         std::string string( const platform::uuid_type& uuid)
         {
            platform::uuid_string_type buffer;
            uuid_unparse_lower( uuid, buffer);

            return buffer;
         }

         std::string string( const Uuid& uuid)
         {
            return string( uuid.get());
         }
      } // uuid


	   const Uuid& Uuid::empty()
	   {
	      static const Uuid empty;
	      return empty;
	   }

	   Uuid Uuid::make()
	   {
	      Uuid result;
	      uuid_generate( result.m_uuid);
	      return result;
	   }

		Uuid::Uuid()
		{
		   memset( &m_uuid, 0, sizeof( m_uuid));
		}

		Uuid::Uuid(uuid_type& uuid)
		{
			uuid_copy( m_uuid, uuid);
		}

		Uuid::Uuid( const std::string& uuid)
		{
		   assert( uuid.size() + 1 == sizeof( platform::uuid_string_type));

		   assert( uuid_parse( uuid.c_str(), m_uuid) == 0);
		}


		const Uuid::uuid_type& Uuid::get() const
		{
			return m_uuid;
		}

      Uuid::uuid_type& Uuid::get()
      {
         return m_uuid;
      }


		void Uuid::copy( uuid_type& uuid) const
		{
			uuid_copy( uuid, m_uuid);
		}

		bool Uuid::operator < ( const Uuid& rhs) const
		{
			return uuid_compare( m_uuid, rhs.m_uuid) < 0;
		}

		bool Uuid::operator == ( const Uuid& rhs) const
		{
			return uuid_compare( m_uuid, rhs.m_uuid) == 0;
		}


      std::ostream& operator << ( std::ostream& out, const Uuid& uuid)
      {
         platform::uuid_string_type buffer;
         uuid_unparse_lower( uuid.m_uuid, buffer);
         out.write( buffer, sizeof( buffer) - 1);
         return out;
      }

      bool operator == ( const Uuid& lhs, const Uuid::uuid_type& rhs)
      {
         return uuid_compare( lhs.get(), rhs) == 0;
      }

      bool operator == ( const Uuid::uuid_type& lhs, const Uuid& rhs)
      {
         return uuid_compare( lhs, rhs.get()) == 0;
      }

   } // common
} // casual




