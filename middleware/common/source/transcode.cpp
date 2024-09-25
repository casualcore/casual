//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transcode.h"

#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"
#include "common/exception/capture.h"

#include "common/result.h"

#include <cppcodec/base64_rfc4648.hpp>

#include <locale>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <iostream>
#include <sstream>
// codecvt is not part of GCC yet ...
//#include <codecvt>
// ... so we have to use the cumbersome iconv instead
#include <iconv.h>
#include <clocale>
#include <cerrno>
#include <cstdlib>
#include <cassert>


namespace casual
{
   namespace common::transcode
   {

      namespace base64
      {
         namespace detail
         {
            platform::size::type encode( const Data source, Data target)
            {
               // calls abort() if target size is insufficient
               return cppcodec::base64_rfc4648::encode(
                  static_cast<char*>(target.memory),
                  target.bytes,
                  static_cast<const char*>(source.memory),
                  source.bytes); 
            }

         platform::size::type decode( std::string_view source, std::span< std::byte> destination)
            {
               try
               {
                  auto char_span = binary::span::to_string_like( destination);

                  // calls abort() if target size is insufficient
                  return cppcodec::base64_rfc4648::decode(
                     char_span.data(),
                     char_span.size(),
                     source.data(),
                     source.size());
               }
               catch( const std::exception& e)
               {
                  code::raise::error( code::casual::failed_transcoding, "base64 decode failed");
               }
            }

         } // detail


         platform::binary::type decode( std::string_view value)
         {
            platform::binary::type result( (value.size() / 4) * 3);

            result.resize( detail::decode( value, result));

            return result;
         }

      } // base64

      namespace local
      {
         namespace
         {
            class converter
            {
            public:
               converter( std::string_view source, std::string_view target)
                  : m_descriptor( iconv_open( target.data(), source.data()))
               {
                  if( m_descriptor == reinterpret_cast< iconv_t>( -1))
                     code::raise::error( code::casual::failed_transcoding, "iconv_open - errc: ", code::system::last::error());
               }

               ~converter()
               {
                  posix::log::result( iconv_close( m_descriptor), "iconv_close");
               }

               auto transcode( const auto& source, auto& target) const
               {
                  static_assert( sizeof( source.front()) == 1);
                  static_assert( sizeof( target.front()) == 1);

                  auto data = const_cast< char*>(reinterpret_cast< const char*>( source.data()));
                  auto size = source.size();

                  do
                  {
                     char buffer[32];
                     auto left = sizeof buffer;
                     char* output = buffer;

                     const auto conversions = iconv( m_descriptor, &data, &size, &output, &left);

                     if( conversions == std::numeric_limits< decltype( conversions)>::max())
                     {
                        switch( auto code = code::system::last::error())
                        {
                           case std::errc::argument_list_too_long: break;
                           default:
                              code::raise::error( code::casual::failed_transcoding, "iconv - errc: ", code);
                        }
                     }

                     target.append( 
                        reinterpret_cast< std::decay_t< decltype( target)>::const_pointer>( buffer), 
                        reinterpret_cast< std::decay_t< decltype( target)>::const_pointer>( output));

                  }while( size);
               }

            private:
               const iconv_t m_descriptor;
            };

            namespace locale::name
            {
               constexpr std::string_view utf8{ "UTF-8"};
               constexpr std::string_view current{ ""};
               
            } // locale::name

         } // 
      } // local

      namespace utf8
      {
         // Make sure this is done once
         //
         // If "" is used as codeset to iconv, it shall choose the current
         // locale-codeset, but that seems to work only if
         // std::setlocale() is called first
         //
         // Perhaps we need to call std::setlocale() each time just in case
         //
         [[maybe_unused]] const auto once = std::setlocale( LC_CTYPE, local::locale::name::current.data());

         std::u8string encode( std::string_view value)
         {
            return encode( value, local::locale::name::current);
         }

         std::string decode( std::u8string_view value)
         {
            return decode( value, local::locale::name::current);
         }

         bool exist( std::string_view codeset)
         {
            try
            {
               local::converter{ codeset, local::locale::name::current};
            }
            catch( ...)
            {
               if( exception::capture().code() == code::casual::failed_transcoding)
                  return false;

               throw;
            }

            return true;
         }

         std::u8string encode( std::string_view value, std::string_view codeset)
         {
            std::u8string result;
            local::converter( codeset, local::locale::name::utf8).transcode( value, result);
            return result;
         }

         std::string decode( std::u8string_view value, std::string_view codeset)
         {
            std::string result;
            local::converter( local::locale::name::utf8, codeset).transcode( value, result);
            return result;
         }

         namespace string
         {
            std::string encode( std::string_view value)
            {
               return encode(value, local::locale::name::current);
            }

            std::string decode( std::string_view value)
            {
               return decode(value, local::locale::name::current);
            }

            std::string encode( std::string_view value, std::string_view codeset)
            {
               std::string result;
               local::converter( codeset, local::locale::name::utf8).transcode( value, result);
               return result;
            }

            std::string decode( std::string_view value, std::string_view codeset)
            {
               std::string result;
               local::converter( local::locale::name::utf8, codeset).transcode( value, result);
               return result;
            }

         } // string

      } // utf8

   } // common::transcode
} // casual



