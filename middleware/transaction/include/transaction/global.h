//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/range.h"
#include "common/transaction/id.h"

#include <array>
#include <iosfwd>

namespace casual
{
   namespace transaction
   {
      namespace global
      {
         struct ID 
         {            
            inline ID( const common::transaction::ID& trid) 
               // gtrid and bqual can only have a value that is at most 64
               : m_size{ static_cast< short>( trid.xid.bqual_length)} 
            {
               common::algorithm::copy( common::transaction::id::range::global( trid), std::begin( m_id));
            }

            inline auto id() const { return common::range::make( std::begin( m_id), m_size);}


            inline friend bool operator == ( const ID& lhs, const ID& rhs) { return lhs.id() == rhs.id();}
            inline friend bool operator == ( const ID& lhs, const common::transaction::ID& rhs) { return lhs.id() == common::transaction::id::range::global( rhs);}
            inline friend bool operator == ( const common::transaction::ID& lhs, const ID& rhs) { return rhs == lhs;}


            friend std::ostream& operator << ( std::ostream& out, const ID& value);

         private:
            short m_size{};
            std::array< char, 64> m_id{};
         };
      } // global
   } // transaction
} // casual