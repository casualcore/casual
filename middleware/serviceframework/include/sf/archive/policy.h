//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_ARCHIVE_POLICY_H_
#define CASUAL_ARCHIVE_POLICY_H_

#include "sf/archive/archive.h"
#include "sf/exception.h"

#include <tuple>

namespace casual
{
   namespace sf
   {
      namespace archive
      {

         namespace policy
         {
            template< typename Implementation>
            struct Strict : Implementation
            {
               using Implementation::Implementation;

               inline static bool apply( bool exist, const char* role)
               {
                  if( ! exist)
                  {
                     throw exception::archive::invalid::Node{ string::compose( "failed to find role in document - role: ", role)};
                  }
                  return exist;
               }

               inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name)
               {
                  auto result = Implementation::container_start( size, name);
                  apply( std::get< 1>( result), name);
                  return result;
               }

               inline bool serialtype_start( const char* name)
               {
                  return apply( Implementation::serialtype_start( name), name);
               }

               template< typename T>
               bool read( T& value, const char* name)
               {
                  return apply( Implementation::read( value, name), name);
               }
            };

            template< typename Implementation>
            struct Relaxed : Implementation
            {
               using Implementation::Implementation;
            };

         } // policy
      } // archive
   } // sf
} // casual

#endif