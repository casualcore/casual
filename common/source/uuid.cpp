//!
//! casual_utility_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "common/uuid.h"

#include <cassert>


namespace casual
{

	namespace common
	{

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

		std::string Uuid::string() const
		{
		   uuid_string_t buffer;
			uuid_unparse_lower( m_uuid, buffer);
			return buffer;
		}

		void Uuid::string( const std::string& value)
		{
		   uuid_string_t buffer;
		   buffer[ sizeof( uuid_string_t) - 1] = '\0';

		   auto end = value.size() < sizeof( uuid_string_t) - 1? value.end() : value.begin() + sizeof( uuid_string_t) - 1;

		   std::copy( value.begin(), end, std::begin( buffer));

		   assert( uuid_parse( buffer, m_uuid) == 0);

		}

		const Uuid::uuid_type& Uuid::get() const
		{
			return m_uuid;
		}

		Uuid::uuid_type& Uuid::get()
      {
         return m_uuid;
      }

		void Uuid::set( uuid_type& uuid)
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

	}
}

bool operator == ( const casual::common::Uuid& lhs, const casual::common::Uuid::uuid_type& rhs)
{
	return uuid_compare( lhs.get(), rhs) == 0;
}

bool operator == ( const casual::common::Uuid::uuid_type& lhs, const casual::common::Uuid& rhs)
{
	return uuid_compare( lhs, rhs.get()) == 0;
}


