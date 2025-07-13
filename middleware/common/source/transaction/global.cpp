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
      namespace local
      {
         namespace
         {
            void validate_hex_string( std::string_view hex_string)
            {
               if( hex_string.size() % 2 == 1)
                  code::raise::error( code::casual::invalid_argument, "invalid format on gtrid: ", hex_string, ", needs to be even length");

               if( hex_string.size() > 128)
                  code::raise::error( code::casual::invalid_argument, "invalid format on gtrid: ", hex_string, ", exceeds maximum size of 64 bytes (128 hex characters)");
            }

            void decode_hex_string( std::string_view hex_string, platform::binary::type& gtrid)
            {
               validate_hex_string( hex_string);
               gtrid.resize( hex_string.size() / 2);

               common::transcode::hex::decode( hex_string, gtrid);
            }
            
         } // <unnamed>
      } // local

      ID::ID( id::range gtrid)
         : m_gtrid( gtrid.data(), gtrid.data() + gtrid.size())
      {
         if( gtrid.size() > 64)
            code::raise::error( code::casual::invalid_argument, "gtrid: ", transcode::hex::encode( gtrid), " has larger size than 64 bytes");
      }

      ID::ID( std::string_view hex_string)
      {
         local::decode_hex_string( hex_string, m_gtrid);
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

         local::decode_hex_string( string, gtrid.m_gtrid);

         return in;
      }
   } // transaction::global
} // casual::common
