//!
//! casual_utility_uuid.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_UUID_H_
#define CASUAL_UTILITY_UUID_H_

#include <uuid/uuid.h>

#include "common/platform.h"

#include <string>

namespace casual
{
	namespace common
	{

		struct Uuid
		{
			typedef platform::uuid_type uuid_type;

			enum Format
			{
			   cNull = -1,
			   cFormatId = 10
			};

			Uuid();
			Uuid( Uuid&&) = default;
			Uuid( const Uuid&) = default;

			Uuid( uuid_type& uuid);

			Uuid( const std::string& uuid);


			Uuid& operator = ( const Uuid&) = default;



			std::string string() const;
			void string( const std::string& value);


			const uuid_type& get() const;
			uuid_type& get();

			//!
			//! Copy to native uuid
			//!
			//! @param uuid target to copy to.
			//!
			void copy( uuid_type& uuid) const;

			bool operator < ( const Uuid& rhs) const;
			bool operator == ( const Uuid& rhs) const;


			static const Uuid& empty();

			static Uuid make();

			static std::string toString( const uuid_type uuid);


		private:
			uuid_type m_uuid;

		};

		inline std::ostream& operator << ( std::ostream& out, const Uuid& uuid)
		{
		   return out << uuid.string();
		}


	} // common
} // casaul

bool operator == ( const casual::common::Uuid& lhs, const casual::common::Uuid::uuid_type& rhs);

bool operator == ( const casual::common::Uuid::uuid_type& rhs, const casual::common::Uuid& lhs);








#endif /* CASUAL_UTILITY_UUID_H_ */
