//!
//! casual
//!


#include "common/uuid.h"
#include "common/transcode.h"
#include "common/memory.h"
#include "common/exception/system.h"
#include "common/string.h"

#include <cassert>
#include <ostream>


namespace casual
{

	namespace common
	{

      namespace uuid
      {
         std::string string( const platform::uuid::type& uuid)
         {
            return transcode::hex::encode( uuid);
         }

         std::string string( const Uuid& uuid)
         {
            return string( uuid.get());
         }

         Uuid make()
         {
            Uuid result;
            uuid_generate( result.get());
            return result;
         }

         const Uuid& empty()
         {
            static const Uuid empty;
            return empty;
         }

      } // uuid


		Uuid::Uuid()
		{
		   memory::set( m_uuid);
		}

		Uuid::Uuid( const uuid_type& uuid)
		{
			uuid_copy( m_uuid, uuid);
		}

		Uuid::Uuid( const std::string& uuid)
		{
		   if( ! uuid.empty())
		   {
	         if( uuid.size() !=  sizeof( uuid_type) * 2)
	         {
	            throw exception::system::invalid::Argument{ string::compose( "invalid uuid string representation: ", uuid)};
	         }

	         transcode::hex::decode( uuid, m_uuid);
		   }
		   else
		   {
            memory::set( m_uuid);
		   }
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

      bool Uuid::empty() const
      {
         return  *this == uuid::empty();
      }

      Uuid::operator bool() const noexcept
      {
         return ! empty();
      }


      bool operator < ( const Uuid& lhs, const Uuid& rhs)
      {
         return uuid_compare( lhs.m_uuid, rhs.m_uuid) < 0;
      }

      bool operator == ( const Uuid& lhs, const Uuid& rhs)
      {
         return uuid_compare( lhs.m_uuid, rhs.m_uuid) == 0;
      }

      bool operator != ( const Uuid& lhs, const Uuid& rhs)
      {
         return ! ( lhs == rhs);
      }

      bool operator == ( const Uuid& lhs, const Uuid::uuid_type& rhs)
      {
         return uuid_compare( lhs.get(), rhs) == 0;
      }

      bool operator == ( const Uuid::uuid_type& lhs, const Uuid& rhs)
      {
         return uuid_compare( lhs, rhs.get()) == 0;
      }


      std::ostream& operator << ( std::ostream& out, const Uuid& uuid)
      {
         return out << transcode::hex::encode( uuid.m_uuid);
      }

   } // common
} // casual




