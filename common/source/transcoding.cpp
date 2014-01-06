//
// transcoding.cpp
//
//  Created on: Dec 26, 2013
//      Author: Kristone
//

#include "common/transcoding.h"

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
      namespace transcoding
      {

         std::string Base64::encode( const std::vector<char>& value)
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

         std::vector<char> Base64::decode( const std::string& value)
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

         namespace
         {
            namespace local
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

         namespace
         {
            namespace local
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
                  std::istringstream stream( std::locale( "").name());

                  locale result;

                  std::getline( stream, result.language, '_');
                  std::getline( stream, result.territory, '.');
                  std::getline( stream, result.codeset, '@');
                  std::getline( stream, result.modifier);

                  return result;
               }
            }

         }


         std::string UTF8::encode( const std::string& value)
         {
/*
            typedef std::wstring::value_type wide_type;
            typedef std::string::value_type tiny_type;

            const std::locale current( "");
            const auto& facet = std::use_facet<std::ctype<wide_type>>(current);
            const auto& widener = [&]( const tiny_type c)
            {
               return facet.widen( c);
            };

            std::wstring wide;
            std::transform(
               value.begin(),
               value.end(),
               std::back_inserter( wide),
               widener);

            // throws std::range_error
            return std::wstring_convert<std::codecvt_utf8<wide_type>,wide_type>().to_bytes( wide);
*/
            // setlocale needs to be called before nl_langinfo works
            //const std::string codeset = nl_langinfo( CODESET);

            return encode( value, local::info().codeset);
         }

         std::string UTF8::decode( const std::string& value)
         {
/*
            typedef std::wstring::value_type wide_type;
            typedef std::string::value_type tiny_type;

            // throws std::range_error
            const auto wide = std::wstring_convert<std::codecvt_utf8<wide_type>,wide_type>().from_bytes( value);

            const std::locale current( "");
            const auto& facet = std::use_facet<std::ctype<wide_type>>(current);
            const auto& narrower = [&]( const wide_type c)
            {
               const auto r = facet.narrow( c, 0);

               if( r) return r;

               if( c) throw std::logic_error( "UTF8-decode failed");

               return r;
            };

            std::string narrow;
            std::transform(
               wide.begin(),
               wide.end(),
               std::back_inserter( narrow),
               narrower);

            return narrow;
*/

            // setlocale needs to be called before nl_langinfo works ...
            //const std::string codeset = nl_langinfo( CODESET);

            return decode( value, local::info().codeset);
         }


         std::string UTF8::encode( const std::string& value, const std::string& codeset)
         {
            return local::converter( codeset, local::UTF8).transcode( value);
         }

         std::string UTF8::decode( const std::string& value, const std::string& codeset)
         {
            return local::converter( local::UTF8, codeset).transcode( value);
         }

      }

   }
}



