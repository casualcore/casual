//!
//! casual
//!

#ifndef SF_LOG_H_
#define SF_LOG_H_

#include "common/log.h"
#include "common/trace.h"

#include "sf/namevaluepair.h"
#include "sf/archive/log.h"


#include <sstream>



namespace casual
{

   namespace sf
   {

      namespace log
      {
         using common::log::debug;
         using common::log::trace;
         using common::log::parameter;
         using common::log::information;
         using common::log::warning;
         using common::log::error;

         extern common::log::Stream sf;

      } // log

      namespace trace
      {
         namespace detail
         {
            class Scope : common::trace::basic::Scope
            {
            public:
               ~Scope();
            protected:
               Scope( const char* information, std::ostream& log);
            };

         } // detail

      } // trace

      struct Trace : trace::detail::Scope
      {
         template<decltype(sizeof("")) size>
         Trace( const char (&information)[size], std::ostream& log = log::sf)
            : trace::detail::Scope( information, log) {}
      };


      template <typename T, typename R>
      std::ostream& operator << ( std::ostream& out, const NameValuePair< T, R>& value)
      {
         if( out.good())
         {
            sf::archive::log::Writer writer( out);
            writer << value;
         }
         return out;
      }

      template <typename T, typename R>
      std::ostream& operator << ( std::ostream& out, NameValuePair< T, R>&& value)
      {
         if( out.good())
         {
            sf::archive::log::Writer writer( out);
            writer << value;
         }
         return out;
      }



   } // sf


} // casual

#endif // SF_LOG_H_
