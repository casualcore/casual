//!
//! casual_utility_uuid.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_UUID_H_
#define CASUAL_UTILITY_UUID_H_

#include <uuid/uuid.h>

#include "casual_utility_platform.h"

#include <string>

namespace casual
{


	namespace utility
	{

		struct Uuid
		{
			typedef platform::uuid_type uuid_type;

			Uuid();
			Uuid( uuid_type& uuid);

			std::string getString() const;
			const uuid_type& get() const;
			void get( uuid_type& uuid);

			bool operator < ( const Uuid& rhs) const;
			bool operator == ( const Uuid& rhs) const;

		private:
			uuid_type m_uuid;

		};

	}
}

bool operator == ( const casual::utility::Uuid& lhs, const casual::utility::Uuid::uuid_type& rhs);

bool operator == ( const casual::utility::Uuid::uuid_type& rhs, const casual::utility::Uuid& lhs);



#endif /* CASUAL_UTILITY_UUID_H_ */
