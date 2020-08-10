//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <uuid/uuid.h>

#include "casual/platform.h"
#include "common/view/string.h"
#include "common/view/binary.h"
#include "common/range.h"

#include "common/serialize/macro.h"
//#include "common/serialize/value.h"

#include <string>

namespace casual
{
   namespace common
   {
      struct Uuid
      {
         using uuid_type = platform::uuid::type;


         Uuid() noexcept = default;

         Uuid( Uuid&& other) noexcept;
         Uuid& operator = ( Uuid&& other) noexcept;

         Uuid( const Uuid&) = default;
         Uuid& operator = ( const Uuid&) = default;

         Uuid( const uuid_type& uuid);
         explicit Uuid( view::String string);

         inline const uuid_type& get() const { return m_uuid;}
         inline uuid_type& get() { return m_uuid;}

         //! Copy to native uuid
         //!
         //! @param uuid target to copy to.
         void copy( uuid_type& uuid) const;

         explicit operator bool() const noexcept;

         bool empty() const;

         // forward serialization
         CASUAL_FORWARD_SERIALIZE( m_uuid)

         friend bool operator < ( const Uuid& lhs, const Uuid& rhs);
         friend bool operator == ( const Uuid& lhs, const Uuid& rhs);
         friend bool operator != ( const Uuid& lhs, const Uuid& rhs);
         friend bool operator == ( const Uuid& lhs, const Uuid::uuid_type& rhs);
         friend bool operator == ( const Uuid::uuid_type& rhs, const Uuid& lhs);

         friend std::ostream& operator << ( std::ostream& out, const Uuid& uuid);
         friend std::istream& operator >> ( std::istream& in, Uuid& uuid);

      private:
         uuid_type m_uuid{};
      };

      namespace uuid
      {
         std::string string( const platform::uuid::type& uuid);
         std::string string( const Uuid& uuid);

         Uuid make();

         inline auto range( const Uuid& uuid) { return range::make( uuid.get());}

         const Uuid& empty();
         bool empty( const Uuid& uuid);
      } // uuid

   } // common

} // casual

casual::common::Uuid operator"" _uuid ( const char* data);














