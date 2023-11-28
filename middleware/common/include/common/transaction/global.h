//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/strong/span.h"
#include "common/serialize/macro.h"
#include "common/algorithm.h"

#include <array>
#include <iosfwd>
#include <string>


namespace casual::common
{
   namespace transaction::global
   {
      namespace id
      {
         struct tag{};

         using range = common::strong::Span< const char, tag>;
      } // id

      struct ID 
      {
         inline ID() = default;
         ID( id::range gtrid);
         ID( const std::string& gtrid);

         inline id::range range() const noexcept { return id::range( m_gtrid.data(), m_gtrid.data() + m_size);}

         inline friend bool operator == ( const ID& lhs, id::range rhs) { return algorithm::equal( lhs.range(), rhs);}
         inline friend bool operator == ( id::range lhs, const ID& rhs) { return rhs == lhs;}
         inline friend bool operator == ( const ID& lhs, const ID& rhs) { return lhs == rhs.range();}
         
         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_size, "size");
            CASUAL_SERIALIZE_NAME( m_gtrid, "gtrid");
         )

         friend std::ostream& operator << ( std::ostream& out, const global::ID& value);
         friend std::istream& operator >> ( std::istream& in, global::ID& gtrid);

      private:
         std::uint8_t m_size{};
         std::array< char, 64> m_gtrid{};           
      };

      // transparent hasher
      struct hash
      {
         using hash_type = std::hash< std::string_view>;
         using is_transparent = void;
         
         inline std::size_t operator()( id::range range) const noexcept
         {
            return hash_type{}( std::string_view( std::data( range), std::size( range)));
         }

         inline std::size_t operator()( const casual::common::transaction::global::ID& value) const noexcept { return hash{}( value.range());}
      };
      
   } // transaction::global
} // casual::common

namespace std 
{
   template<>
   struct hash< casual::common::transaction::global::ID>
   {
      std::size_t operator()( const casual::common::transaction::global::ID& value) const noexcept
      {
         return casual::common::transaction::global::hash{}( value);
      }
   };
}
