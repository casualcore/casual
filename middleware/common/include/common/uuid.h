//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
			typedef platform::uuid::type uuid_type;


			Uuid();
			Uuid( Uuid&&) noexcept = default;
			Uuid& operator = ( Uuid&&) noexcept = default;

			Uuid( const Uuid&) = default;
         Uuid& operator = ( const Uuid&) = default;

			Uuid( const uuid_type& uuid);
			Uuid( const std::string& uuid);


			const uuid_type& get() const;
			uuid_type& get();

			//!
			//! Copy to native uuid
			//!
			//! @param uuid target to copy to.
			//!
			void copy( uuid_type& uuid) const;

			explicit operator bool() const noexcept;

			bool empty() const;




			friend std::ostream& operator << ( std::ostream& out, const Uuid& uuid);

         friend bool operator < ( const Uuid& lhs, const Uuid& rhs);
         friend bool operator == ( const Uuid& lhs, const Uuid& rhs);
         friend bool operator != ( const Uuid& lhs, const Uuid& rhs);
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
         std::string string( const platform::uuid::type& uuid);
         std::string string( const Uuid& uuid);

         Uuid make();

         const Uuid& empty();


      } // uuid

	} // common

} // casaul













#endif /* CASUAL_UTILITY_UUID_H_ */
