//!
//! casual
//!

#ifndef SF_LOG_H_
#define SF_LOG_H_

#include "common/log/category.h"
#include "common/log.h"
#include "common/log/stream.h"
#include "common/traits.h"

#include "sf/namevaluepair.h"
#include "sf/archive/log.h"
#include "sf/archive/line.h"


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

   namespace common
   {
      namespace log
      {
         namespace detail
         {
            template< typename T>
            using can_serialize = decltype( std::declval< T&>().serialize( std::declval< sf::archive::Writer&>()));
         } // detail

         //!
         //! Specialization for containers, to log ranges
         //!
         template< typename T> 
         struct has_formatter< T, std::enable_if_t< 
               sf::traits::detect::is_detected< detail::can_serialize, T>::value>>
            : std::true_type
         {
            struct formatter
            {
               template< typename V>
               void operator () ( std::ostream& out, V&& value) const
               { 
                  auto writer = sf::archive::line::writer( out);
                  writer << sf::name::value::pair::make( nullptr, std::forward< V>( value));
               }
            };
         };
         
      } // log
   } // common
} // casual

#endif // SF_LOG_H_
