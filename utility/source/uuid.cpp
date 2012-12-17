//!
//! casual_utility_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "utility/uuid.h"

namespace casual
{

	namespace utility
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

		std::string Uuid::getString() const
		{
			char buffer[ 37];
			uuid_unparse_lower( m_uuid, buffer);
			return buffer;
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

bool operator == ( const casual::utility::Uuid& lhs, const casual::utility::Uuid::uuid_type& rhs)
{
	return uuid_compare( lhs.get(), rhs) == 0;
}

bool operator == ( const casual::utility::Uuid::uuid_type& lhs, const casual::utility::Uuid& rhs)
{
	return uuid_compare( lhs, rhs.get()) == 0;
}


