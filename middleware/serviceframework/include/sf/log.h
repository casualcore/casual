//!
//! casual
//!

#ifndef SF_LOG_H_
#define SF_LOG_H_

#include "common/log/category.h"
#include "common/log.h"
#include "common/traits.h"

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
         using common::log::category::parameter;
         using common::log::category::information;
         using common::log::category::warning;
         using common::log::category::error;

         extern common::log::Stream sf;

      } // log

      namespace trace
      {
         namespace detail
         {
            class Scope : common::log::trace::basic::Scope
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

      namespace name
      {
         namespace value
         {
            template <typename NVP>
            auto operator << ( std::ostream& out, NVP&& value) -> common::traits::enable_if_t< sf::traits::is_nvp< NVP>::value, std::ostream&>
            {
               if( out.good())
               {
                  auto archive = sf::archive::log::writer( out);
                  archive << value;
               }
               return out;
            }

         } // value
      } // name
   } // sf
} // casual

#endif // SF_LOG_H_
