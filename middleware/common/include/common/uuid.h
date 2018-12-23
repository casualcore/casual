//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <uuid/uuid.h>

#include "common/platform.h"
#include "common/view/string.h"

#include <string>

namespace casual
{
   namespace common
   {
      struct Uuid
      {
         using uuid_type = platform::uuid::type;


         Uuid() = default;
         Uuid( Uuid&&) noexcept = default;
         Uuid& operator = ( Uuid&&) noexcept = default;

         Uuid( const Uuid&) = default;
         Uuid& operator = ( const Uuid&) = default;

         Uuid( const uuid_type& uuid);
         explicit Uuid( view::String string);



         inline const uuid_type& get() const { return m_uuid;}
         inline uuid_type& get() { return m_uuid;}

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
         uuid_type m_uuid = {};
      };

      namespace uuid
      {
         std::string string( const platform::uuid::type& uuid);
         std::string string( const Uuid& uuid);

         Uuid make();

         const Uuid& empty();
         bool empty( const Uuid& uuid);
      } // uuid
   } // common
} // casual














