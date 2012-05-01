//!
//! casual_utility_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "casual_utility_uuid.h"

namespace casual
{

	namespace utility
	{
		uuid::uuid()
		{
			uuid_generate( m_uuid);
		}

		std::string uuid::getString() const
		{
			uuid_string_t buffer;
			uuid_unparse_lower( m_uuid, buffer);
			return buffer;
		}

		/*
		std::string uuid::generate()
		{
			uuid result;
			return result.getString();
		}
		*/

		const uuid_t& uuid::get() const
		{
			return m_uuid;
		}

		bool uuid::operator < ( const uuid& rhs) const
		{
			return uuid_compare( m_uuid, rhs.m_uuid) < 0;
		}

		bool uuid::operator == ( const uuid& rhs) const
		{
			return uuid_compare( m_uuid, rhs.m_uuid) == 0;
		}

	}
}


