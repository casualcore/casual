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
		Uuid::Uuid()
		{
			uuid_generate( m_uuid);
		}

		std::string Uuid::getString() const
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

		const uuid_t& Uuid::get() const
		{
			return m_uuid;
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


