//!
//! casual_utility_uuid.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_UUID_H_
#define CASUAL_UTILITY_UUID_H_

#include <uuid/uuid.h>

#include <string>

namespace casual
{
	namespace platform
	{
		typedef uuid_t uuid_type;
	}

	namespace utility
	{

		struct Uuid
		{
			typedef platform::uuid_type uuid_type;

			Uuid();

			std::string getString() const;
			const uuid_t& get() const;

			bool operator < ( const Uuid& rhs) const;
			bool operator == ( const Uuid& rhs) const;

		private:
			uuid_type m_uuid;

		};

	}
}



#endif /* CASUAL_UTILITY_UUID_H_ */
