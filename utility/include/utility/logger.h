//!
//! casual_logger.h
//!
//! Created on: Jun 21, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_LOGGER_H_
#define CASUAL_LOGGER_H_

#include <string>

//
// TODO: Temp, we will not use iostream later on!
//
#include <sstream>

#include "utility/platform.h"

namespace casual
{
   namespace utility
   {
      namespace logger
      {

         namespace internal
         {
            //
            // Proxy-type that logs when full expression has ended.
            //
            class Proxy
            {
            public:

               Proxy( const int priority);
               Proxy( const Proxy&) = delete;
               Proxy& operator = ( const Proxy&) = delete;

               //
               // We can't rely on RVO, so we have to release logging-responsibility for
               // rhs
               //
               Proxy( Proxy&& rhs);

               //
               // Will be called when the full expression has "run", and this rvalue
               // will be destructed
               //
               ~Proxy();

               template< typename T>
               Proxy& operator << ( T&& value)
               {
                  m_message << std::forward< T>( value);
                  return *this;
               }

            private:
               //
               // TODO: We should use something else, hence not have any
               // dependencies to iostream
               //
               std::ostringstream m_message;
               const int m_priority;
               bool m_log;
            };

            template< int priority>
            class basic_logger
            {
            public:

               //!
               //! @return proxy-type that logs when full expression has ended.
               //!
               template< typename T>
               Proxy operator <<( T&& value)
               {
                  Proxy proxy( priority);
                  proxy << std::forward< T>( value);
                  return proxy;
               }

            };

         } // internal

         extern internal::basic_logger< utility::platform::cLOG_debug> debug;

         extern internal::basic_logger< utility::platform::cLOG_debug> trace;

         extern internal::basic_logger< utility::platform::cLOG_info> information;

         extern internal::basic_logger< utility::platform::cLOG_warning> warning;

         extern internal::basic_logger< utility::platform::cLOG_error> error;

      } // logger

   } // utility

} // casual

#endif /* CASUAL_LOGGER_H_ */
