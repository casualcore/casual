//
// transcoding.h
//
//  Created on: Dec 26, 2013
//      Author: Kristone
//

#ifndef TRANSCODING_H_
#define TRANSCODING_H_

#include <string>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace transcode
      {
         struct Base64
         {
            //
            // @return Base64-encoded binary data
            //
            // @throw std::logic_error on failure
            //
            static std::string encode( const std::vector<char>& value);

            //
            // @return Base64-decoded binary data
            //
            // @throw std::logic_error on failure
            //
            static std::vector<char> decode( const std::string& value);
         };

         struct UTF8
         {
            //
            // @param value String encoded in local default codeset
            //
            // @return UTF-8-encoded string
            //
            // @throw std::logic_error on some failures
            // @throw std::runtime_error on some failures
            //
            static std::string encode( const std::string& value);

            //
            // @param value The UTF-8 encoded string
            //
            // @return String encoded in local default codeset
            //
            // @throw std::logic_error on some failures
            // @throw std::runtime_error on some failures
            //
            static std::string decode( const std::string& value);

            //
            // @param value String encoded in provided codeset
            // @param codeset String-encoding
            //
            // @return UTF-8-encoded string
            //
            // @throw std::logic_error on some failures
            // @throw std::runtime_error on some failures
            //
            static std::string encode( const std::string& value, const std::string& codeset);

            //
            // @param value The UTF-8 encoded string
            // @param codeset Encoding for result
            //
            // @return String encoded in provided codeset
            //
            // @throw std::logic_error on some failures
            // @throw std::runtime_error on some failures
            //
            static std::string decode( const std::string& value, const std::string& codeset);
         };

      }
   }
}


#endif /* TRANSCODING_H_ */
