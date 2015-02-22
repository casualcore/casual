//
// transcoding.h
//
//  Created on: Dec 26, 2013
//      Author: Kristone
//

#ifndef CASUAL_COMMON_TRANSCODE_H_
#define CASUAL_COMMON_TRANSCODE_H_


#include "common/platform.h"

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

         namespace hex
         {
            namespace detail
            {
               std::string encode( const void* data, std::size_t bytes);
               void decode( const std::string& value, void* data);
            } // detail


            template< typename Iter>
            std::string encode( Iter first, Iter last)
            {
               return detail::encode( &(*first), last - first);
            }

            template< typename C>
            std::string encode( C&& container)
            {
               return encode( std::begin( container), std::end( container));
            }


            platform::binary_type decode( const std::string& value);


            template< typename C>
            void decode( const std::string& value, C&& container)
            {
               return detail::decode( value, &( *std::begin( container)));
            }




         } // hex
      } // transcode
   } // common
} // casual


#endif /* TRANSCODING_H_ */
