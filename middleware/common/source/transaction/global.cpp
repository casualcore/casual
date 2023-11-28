//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/transaction/global.h"

#include "common/transcode.h"

#include "common/code/raise.h"

#include "casual/assert.h"

#include <iostream>
#include <ostream>
#include <regex>

namespace casual::common
{
   namespace transaction::global
   {
      ID::ID( id::range gtrid)
      {
         assertion( gtrid.size() <= 64, "trid: ", transcode::hex::encode( gtrid), " has larger gtrid size than 64");

         m_size = gtrid.size();
         common::algorithm::copy( gtrid, std::begin( m_gtrid));
      }

      ID::ID( const std::string& gtrid)
      {
         common::transcode::hex::decode( gtrid, m_gtrid);
         m_size = gtrid.size() / 2;
      }


      std::ostream& operator << ( std::ostream& out, const ID& value)
      {         
         return common::transcode::hex::encode( out, value.range());
      }

      std::istream& operator >> ( std::istream& in, common::transaction::global::ID& gtrid)
      {
         std::string string;
         in >> string;

         if( ! std::regex_match( string, std::regex{ "[a-f0-9]+"}))
            code::raise::error( code::casual::invalid_argument, "invalid format on gtrid: ", string);

         if( string.size() % 2 == 1)
            code::raise::error( code::casual::invalid_argument, "invalid format on gtrid: ", string, ", needs to be even length");

         assertion( string.size() <= 64, "trid: ", string, " has larger gtrid size than 64");

         common::transcode::hex::decode( string, gtrid.m_gtrid);
         gtrid.m_size = string.size() / 2;

         return in;
      }
   } // transaction::global
} // casual::common