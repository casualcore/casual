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

namespace casual
{
   namespace transaction::global
   {
      struct ID 
      {
         inline ID() = default;
         ID( const common::transaction::ID& trid); 

         inline auto operator () () const noexcept { return common::view::binary::make( std::begin( m_gtrid), m_size);}

         inline friend bool operator == ( const ID& lhs, const ID& rhs) { return lhs() == rhs();}
         inline friend bool operator == ( const ID& lhs, const common::transaction::ID& rhs) { return lhs() == common::transaction::id::range::global( rhs);}
         inline friend bool operator == ( const common::transaction::ID& lhs, const ID& rhs) { return rhs == lhs;}


         friend std::ostream& operator << ( std::ostream& out, const ID& value);

      private:
         std::int8_t m_size{};
         std::array< char, 64> m_gtrid{};           
      };
      
   } // transaction::global
} // casual