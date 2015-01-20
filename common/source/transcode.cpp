//
// transcoding.cpp
//
//  Created on: Dec 26, 2013
//      Author: Kristone
//

#include "common/transcode.h"

#include <resolv.h>

#include <stdexcept>
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
#include <cstring>
#include <cstdlib>
//#include <langinfo.h>


namespace casual
{
   namespace common
   {
      namespace transcode
      {

         namespace base64
         {
            std::string encode( const std::vector<char>& value)
            {
               //
               // b64_ntop requires one extra char of some reason
               //
               std::string result( ((value.size() + 2) / 3) * 4 + 1, 0);

               const auto length =
                  b64_ntop(
                     reinterpret_cast<const unsigned char*>(value.data()),
                     value.size(),
                     &result[0],
                     result.size());

               if( length < 0)
               {
                  throw std::logic_error( "Base64-encode failed");
               }

               result.resize( length);

               return result;
            }

            std::vector<char> decode( const std::string& value)
            {
               std::vector<char> result( (value.size() / 4) * 3);

               const auto length =
                  b64_pton(
                     value.data(),
                     reinterpret_cast<unsigned char*>(result.data()),
                     result.size());

               if( length < 0)
               {
                  throw std::logic_error( "Base64-decode failed");
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
                     if( m_descriptor == reinterpret_cast<iconv_t>( -1))
                     {
                        switch( errno)
                        {
                        case EMFILE:
                        case ENFILE:
                        case ENOMEM:
                           throw std::runtime_error( std::strerror( errno));
                        case EINVAL:
                        default:
                           throw std::logic_error( std::strerror( errno));
                        }
                     }
                  }

                  ~converter()
                  {
                     if( iconv_close( m_descriptor) == -1)
                     {
                        std::cerr << std::strerror( errno) << std::endl;
                     }
                  }

                  std::string transcode( const std::string& value) const
                  {
                     char* source = const_cast<char*>(value.c_str());
                     std::size_t size = value.size();

                     std::string result;

                     do
                     {
                        char buffer[32];
                        std::size_t left = sizeof buffer;
                        char* target = buffer;

                        const auto conversions = iconv( m_descriptor, &source, &size, &target, &left);

                        if( conversions == std::numeric_limits< decltype( conversions)>::max())
                        {
                           switch( errno)
                           {
                           case E2BIG:
                              break;
                           case EILSEQ:
                           case EINVAL:
                           case EBADF:
                           default:
                              throw std::logic_error( std::strerror( errno));
                           }
                        }

                        result.append( buffer, target);

                     }while( size);

                     return result;
                  }

               private:
                  const iconv_t m_descriptor;
               };

            }
         }

         namespace local
         {
            namespace
            {
               struct locale
               {
                  std::string language;
                  std::string territory;
                  std::string codeset;
                  std::string modifier;
               };

               locale parse( const std::string& name)
               {
                  std::istringstream stream( name);

                  locale result;

                  std::getline( stream, result.language, '_');
                  std::getline( stream, result.territory, '.');
                  std::getline( stream, result.codeset, '@');
                  std::getline( stream, result.modifier);

                  return result;
               }

               locale info()
               {
                  //return parse( std::locale( "").name());
                  return parse( std::setlocale( LC_CTYPE, ""));
               }
            }

         }

         namespace utf8
         {
            const std::string cUTF8( "UTF-8");

            std::string encode( const std::string& value)
            {
               //return encode( value, "");
               return encode( value, local::info().codeset);
            }

            std::string decode( const std::string& value)
            {
               //return decode( value, "");
               return decode( value, local::info().codeset);
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
                        auto hex = []( decltype( *first) value){
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
               } // <unnamed>
            } // local
            std::string encode( const void* data, std::size_t bytes)
            {
               std::string result;
               result.reserve( bytes * 2);

               const char* first = static_cast< const char*>( data);
               auto last = first + bytes;

               local::encode( first, last, std::back_inserter( result));

               return result;
            }

         } // hex

      } // transcode
   } // common
} // casual



