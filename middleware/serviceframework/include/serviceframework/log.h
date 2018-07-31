//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/log/category.h"
#include "common/log.h"
#include "common/log/stream.h"
#include "common/traits.h"

#include "serviceframework/namevaluepair.h"
#include "serviceframework/archive/log.h"
#include "serviceframework/archive/line.h"


#include <sstream>



namespace casual
{

   namespace serviceframework
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
            auto operator << ( std::ostream& out, NVP&& value) -> std::enable_if_t< traits::is_nvp< NVP>::value, std::ostream&>
            {
               if( out.good())
               {
                  auto archive = archive::log::writer( out);
                  archive << value;
               }
               return out;
            }

         } // value
      } // name
   } // serviceframework

   namespace common
   {
      namespace stream
      {
         namespace detail
         {
            template< typename T>
            using can_serialize = decltype( std::declval< T&>().serialize( std::declval< serviceframework::archive::Writer&>()));
         } // detail

         //!
         //! Specialization for containers, to log ranges
         //!
         template< typename T> 
         struct has_formatter< T, std::enable_if_t< 
               traits::detect::is_detected< detail::can_serialize, T>::value>>
            : std::true_type
         {
            struct formatter
            {
               template< typename V>
               void operator () ( std::ostream& out, V&& value) const
               { 
                  auto writer = serviceframework::archive::line::writer( out);
                  writer << serviceframework::name::value::pair::make( nullptr, std::forward< V>( value));
               }
            };
         };
         
      } // stream
   } // common
} // casual


