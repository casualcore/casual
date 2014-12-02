//
// transcoding.h
//
//  Created on: Dec 26, 2013
//      Author: Kristone
//

#ifndef CASUAL_COMMON_TRANSCODE_H_
#define CASUAL_COMMON_TRANSCODE_H_

#include <string>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace transcode
      {
         namespace base64
         {
            //!
            //! @return Base64-encoded binary data
            //!
            //! @throw std::logic_error on failure
            //!
            std::string encode( const std::vector<char>& value);

            //!
            //! @return Base64-decoded binary data
            //!
            //! @throw std::logic_error on failure
            //!
            std::vector<char> decode( const std::string& value);

         } // base64

         namespace utf8
         {
            //!
            //! @param value String encoded in local default codeset
            //!
            //! @return UTF-8-encoded string
            //!
            //! @throw std::logic_error on some failures
            //! @throw std::runtime_error on some failures
            //!
            std::string encode( const std::string& value);

            //!
            //! @param value The UTF-8 encoded string
            //!
            //! @return String encoded in local default codeset
            //!
            //! @throw std::logic_error on some failures
            //! @throw std::runtime_error on some failures
            //!
            std::string decode( const std::string& value);

            //!
            //! @param value String encoded in provided codeset
            //! @param codeset String-encoding
            //!
            //! @return UTF-8-encoded string
            //!
            //! @throw std::logic_error on some failures
            //! @throw std::runtime_error on some failures
            //!
            std::string encode( const std::string& value, const std::string& codeset);

            //!
            //! @param value The UTF-8 encoded string
            //! @param codeset Encoding for result
            //!
            //! @return String encoded in provided codeset
            //!
            //! @throw std::logic_error on some failures
            //! @throw std::runtime_error on some failures
            //!
            std::string decode( const std::string& value, const std::string& codeset);

         } // utf8

      } // transcode
   } // common
} // casual


#endif /* TRANSCODING_H_ */
