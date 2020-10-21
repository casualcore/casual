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
            inline ID()= default;
            inline ID( const common::transaction::ID& trid) 
               : trid{ trid} {}

            inline auto global() const { return common::transaction::id::range::global( trid);}

            inline friend bool operator == ( const ID& lhs, const ID& rhs) { return lhs.global() == rhs.global();}
            inline friend bool operator == ( const ID& lhs, const common::transaction::ID& rhs) { return lhs.global() == common::transaction::id::range::global( rhs);}
            inline friend bool operator == ( const common::transaction::ID& lhs, const ID& rhs) { return rhs == lhs;}


            friend std::ostream& operator << ( std::ostream& out, const ID& value);

            common::transaction::ID trid;            
         };
      } // global
   } // transaction
} // casual