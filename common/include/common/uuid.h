//!
//! casual_utility_uuid.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_UUID_H_
#define CASUAL_COMMON_UUID_H_

#include <uuid/uuid.h>

#include "common/platform.h"
//#include "common/marshal.h"

#include <string>

namespace casual
{
	namespace common
	{

		struct Uuid
		{
			typedef platform::uuid_type uuid_type;


			Uuid();
			Uuid( Uuid&&) = default;
			Uuid( const Uuid&) = default;

			Uuid( uuid_type& uuid);

			Uuid( const std::string& uuid);


			Uuid& operator = ( const Uuid&) = default;



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


			friend std::ostream& operator << ( std::ostream& out, const Uuid& uuid);

			friend bool operator == ( const Uuid& lhs, const Uuid::uuid_type& rhs);

			friend bool operator == ( const Uuid::uuid_type& rhs, const Uuid& lhs);


		   template< typename A>
		   void marshal( A& archive)
		   {
		      archive & m_uuid;
		   }

		   template< typename A>
		   void marshal( A& archive) const
		   {
		      archive & m_uuid;
		   }


		private:
			uuid_type m_uuid;

		};

      namespace uuid
      {
         std::string string( const platform::uuid_type& uuid);
         std::string string( const Uuid& uuid);
      } // uuid

	} // common
} // casaul













#endif /* CASUAL_UTILITY_UUID_H_ */
