//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/view/binary.h"
#include "common/transaction/id.h"

#include <array>
#include <iosfwd>
#include <string>


namespace casual::common
{
   namespace transaction::global
   {
      struct ID 
      {
         inline ID() = default;
         ID( const common::transaction::ID& trid);
         ID( const std::string& trid);

         inline auto operator () () const noexcept { return common::view::binary::make( std::begin( m_gtrid), m_size);}

         inline friend bool operator == ( const ID& lhs, const ID& rhs) { return algorithm::equal( lhs(), rhs());}
         inline friend bool operator == ( const ID& lhs, const common::transaction::ID& rhs) { return algorithm::equal( lhs(), common::transaction::id::range::global( rhs));}
         inline friend bool operator == ( const common::transaction::ID& lhs, const ID& rhs) { return rhs == lhs;}

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_size, "size");
            CASUAL_SERIALIZE_NAME( m_gtrid, "gtrid");
         )

         auto span() const noexcept
         {
            return std::span< const char>( m_gtrid.data(), m_size);
         }

         friend std::ostream& operator << ( std::ostream& out, const ID& value);
         friend std::istream& operator >> ( std::istream& in, common::transaction::global::ID& gtrid);

      private:
         std::uint8_t m_size{};
         std::array< char, 64> m_gtrid{};           
      };

      namespace to
      {
         common::transaction::ID trid( const global::ID& gtrid);
      } // to

      
      
   } // transaction::global
} // casual::common

namespace std 
{
   template<>
   struct hash< casual::common::transaction::global::ID>
   {
      auto operator()( const casual::common::transaction::global::ID& value) const noexcept
      {
         auto span = value.span();
         
         // we just use the string_view hash function.
         auto string_representation = std::string_view{ span.data(), span.size()};
         return std::hash< std::string_view>{}( string_representation);
      }
   };
}
