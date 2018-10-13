//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/transcode.h"

#include "common/exception/system.h"
#include "common/code/system.h"


#include <resolv.h>

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
   namespace common
   {
      namespace transcode
      {

         namespace base64
         {
            namespace detail
            {

               platform::size::type encode( const Data source, Data target)
               {
                  const auto length =
                     b64_ntop(
                        static_cast<const unsigned char*>( source.memory),
                        source.bytes,
                        static_cast< char*>( target.memory),
                        target.bytes);

                  if( length < 0)
                  {
                     throw exception::system::invalid::Argument( "Base64-encode failed");
                  }

                  return length;
               }

            } // detail


            platform::binary::type decode( const std::string& value)
            {
               std::vector<char> result( (value.size() / 4) * 3);

               const auto length =
                  b64_pton(
                     value.data(),
                     reinterpret_cast<unsigned char*>(result.data()),
                     result.size());

               if( length < 0)
               {
                  throw exception::system::invalid::Argument( "Base64-decode failed");
               }

               result.resize( length);

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
                  converter( const std::string& source, const std::string& target)
                     : m_descriptor( iconv_open( target.c_str(), source.c_str()))
                  {
                     if( m_descriptor == reinterpret_cast< iconv_t>( -1))
                     {
                        exception::system::throw_from_errno();
                     }

                  }

                  ~converter()
                  {
                     if( iconv_close( m_descriptor) == -1)
                     {
                        std::cerr << std::make_error_code( code::last::system::error()) << '\n';
                     }
                  }

                  std::string transcode( const std::string& value) const
                  {
                     auto source = const_cast<char*>( value.c_str());
                     auto size = value.size();

                     std::string result;

                     do
                     {
                        char buffer[32];
                        auto left = sizeof buffer;
                        char* target = buffer;

                        const auto conversions = iconv( m_descriptor, &source, &size, &target, &left);

                        if( conversions == std::numeric_limits< decltype( conversions)>::max())
                        {
                           switch( code::last::system::error())
                           {
                              case code::system::argument_list_too_long: break;
                              default:
                                 exception::system::throw_from_errno();
                           }
                          
                        }

                        result.append( buffer, target);

                     }while( size);

                     return result;
                  }

               private:
                  const iconv_t m_descriptor;
               };

               struct locale
               {
                  std::string language;
                  std::string territory;
                  std::string codeset;
                  std::string modifier;
               };
            }

         }

         namespace utf8
         {
            const std::string cUTF8( "UTF-8");
            const std::string cCurrent( "");

            //
            // Make sure this is done once
            //
            // If "" is used as codeset to iconv, it shall choose the current
            // locale-codeset, but that seems to work only if
            // std::setlocale() is called first
            //
            // Perhaps we need to call std::setlocale() each time just in case
            //
            //[[maybe_unused]] const auto once = std::setlocale( LC_CTYPE, "");
#ifdef __GNUC__
            __attribute__((__unused__)) const auto once = std::setlocale( LC_CTYPE, "");
#else
            const auto once = std::setlocale( LC_CTYPE, "");
#endif
            std::string encode( const std::string& value)
            {
               //return encode( value, local::info().codeset);
               return encode( value, cCurrent);
            }

            std::string decode( const std::string& value)
            {
               //return decode( value, local::info().codeset);
               return decode( value, cCurrent);
            }

            bool exist( const std::string& codeset)
            {
               try
               {
                  local::converter{ codeset, cCurrent};
               }
               catch( const exception::system::invalid::Argument&)
               {
                  return false;
               }

               return true;
            }

            std::string encode( const std::string& value, const std::string& codeset)
            {
               return local::converter( codeset, cUTF8).transcode( value);
            }

            std::string decode( const std::string& value, const std::string& codeset)
            {
               return local::converter( cUTF8, codeset).transcode( value);
            }

         } // utf8

         namespace hex
         {
            namespace local
            {
               namespace
               {
                  template< typename InIter, typename OutIter>
                  void encode( InIter first, InIter last, OutIter out)
                  {
                     while( first != last)
                     {
                        auto hex = []( auto value){
                           if( value < 10)
                           {
                              return value + 48;
                           }
                           return value + 87;
                        };

                        *out++ = hex( ( 0xf0 & *first) >> 4);
                        *out++ = hex( 0x0f & *first);

                        ++first;
                     }
                  }

                  template< typename InIter, typename OutIter>
                  void decode( InIter first, InIter last, OutIter out)
                  {
                     assert( ( last - first) % 2 == 0);

                     while( first != last)
                     {
                        auto hex = []( decltype( *first) value)
                        {
                           if( value >= 87)
                           {
                              return value - 87;
                           }
                           return value - 48;
                        };

                        *out = ( 0x0F & hex( *first++)) << 4;
                        *out += 0x0F & hex( *first++);

                        ++out;
                     }
                  }

               } // <unnamed>
            } // local

            namespace detail
            {
               std::string encode( const char* first, const char* last)
               {
                  auto bytes = last - first;
                  std::string result( bytes * 2, 0);

                  local::encode( first, last, result.begin());

                  return result;
               }

               void encode( std::ostream& out, const char* first, const char* last)
               {
                  local::encode( first, last, std::ostream_iterator< const char>( out));
               }

               void decode(const char* first, const char* last, char* data)
               {
                  local::decode( first, last, data);
               }
               

            } // detail


            platform::binary::type decode( const std::string& value)
            {
               platform::binary::type result( value.size() / 2);

               local::decode( value.begin(), value.end(), result.begin());

               return result;
            }

         } // hex

      } // transcode
   } // common
} // casual



