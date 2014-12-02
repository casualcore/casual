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
// codecvt is not part of GCC yet ...
//#include <codecvt>
// ... so we have to use the cumbersome iconv instead
#include <iconv.h>
#include <cerrno>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
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
                        case_EMFILE:
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

                        if( conversions == -1)
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

               const std::string UTF8( "UTF-8");
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

               locale info()
               {
                  std::istringstream stream( std::locale().name());

                  locale result;

                  std::getline( stream, result.language, '_');
                  std::getline( stream, result.territory, '.');
                  std::getline( stream, result.codeset, '@');
                  std::getline( stream, result.modifier);

                  return result;
               }
            }

         }

         namespace utf8
         {
            std::string encode( const std::string& value)
            {
               return encode( value, local::info().codeset);
            }

            std::string decode( const std::string& value)
            {
               return decode( value, local::info().codeset);
            }


            std::string encode( const std::string& value, const std::string& codeset)
            {
               return local::converter( codeset, local::UTF8).transcode( value);
            }

            std::string decode( const std::string& value, const std::string& codeset)
            {
               return local::converter( local::UTF8, codeset).transcode( value);
            }

         } // utf8
      }

   }
}



