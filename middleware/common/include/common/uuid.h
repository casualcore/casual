//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <uuid/uuid.h>

#include "casual/platform.h"
#include "common/view/binary.h"
#include "common/range.h"
#include "common/algorithm.h"

#include "common/serialize/macro.h"

#include <string>
#include <string_view>

namespace casual
{
   namespace common
   {
      struct Uuid
      {
         using uuid_type = platform::uuid::type;

         Uuid() noexcept = default;

         explicit Uuid( const uuid_type& uuid);
         explicit Uuid( std::string_view string);

         inline const uuid_type& get() const { return m_uuid;}
         inline uuid_type& get() { return m_uuid;}

         inline auto range() const noexcept { return  view::binary::make( m_uuid);}

         //! Copy to native uuid
         //!
         //! @param uuid target to copy to.
         void copy( uuid_type& uuid) const;

         explicit operator bool() const noexcept;

         bool empty() const;

         // forward serialization
         CASUAL_FORWARD_SERIALIZE( view::binary::make( m_uuid))

         friend inline bool operator == ( const Uuid& lhs, const Uuid& rhs) noexcept { return algorithm::equal( lhs.m_uuid, rhs.m_uuid);}     
         friend inline bool operator < ( const Uuid& lhs, const Uuid& rhs) noexcept { return algorithm::lexicographical::compare( lhs.m_uuid, rhs.m_uuid);}

         friend inline bool operator == ( const Uuid& lhs, const Uuid::uuid_type& rhs) noexcept { return algorithm::equal( lhs.m_uuid, rhs);}    


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


namespace std 
{
   template<>
   struct hash< casual::common::Uuid>
   {
     auto operator()( const casual::common::Uuid& value) const 
     {
         static_assert( sizeof( value.get()) == 2 * sizeof( std::uint64_t));

         // not sure if this is good enough hash... It should be since the uuid 
         // in it self have real good entropy.

         auto data_64 = reinterpret_cast< const std::uint64_t*>( &value.get());
         return data_64[ 0] ^ data_64[ 1];
     }
   };
}

casual::common::Uuid operator"" _uuid ( const char* data);














