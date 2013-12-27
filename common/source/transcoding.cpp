//
// transcoding.cpp
//
//  Created on: Dec 26, 2013
//      Author: Kristone
//

#include <resolv.h>
#include <stdexcept>

#include "common/transcoding.h"

/*
#include <iostream>
#include <cerrno>
#include <cstring>
*/

namespace casual
{
   namespace common
   {
      namespace transcoding
      {

         namespace Base64
         {
            std::string encode( const std::vector<char>& data)
            {
               //
               // b64_ntop requires one extra char of some reason
               //
               std::string result( ((data.size() + 2) / 3) * 4 + 1, 0);

               const auto length =
                  b64_ntop(
                     reinterpret_cast<const unsigned char*>(data.data()),
                     data.size(),
                     &result[0],
                     result.size());

               if( length < 0)
               {
                  throw std::logic_error( "Base64-encode failed");
               }

               result.resize( length);

               return result;
            }

            std::vector<char> decode( const std::string& data)
            {
               std::vector<char> result( (data.size() / 4) * 3);

               const auto length =
                  b64_pton(
                     data.data(),
                     reinterpret_cast<unsigned char*>(result.data()),
                     result.size());

               if( length < 0)
               {
                  throw std::logic_error( "Base64-decode failed");
               }

               result.resize( length);

               return result;

            }
         }
      }

   }
}



