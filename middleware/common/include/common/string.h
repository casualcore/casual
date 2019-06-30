//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/platform.h"
#include "common/traits.h"
#include "common/algorithm.h"
#include "common/stream.h"
#include "common/view/string.h"

#include <string>
#include <locale>

#include <regex>
#include <algorithm>
#include <sstream>

#include <cstdlib>

namespace casual
{
   namespace common
   {
      namespace string
      {
         //! splits a range
         //!
         //! @param line line to be splittet
         //! @param delimiter the value to use as a splitter
         //!
         //! @return the splitted range.
         std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter = ' ');


         namespace adjacent
         {
            //! splits a range, and ignores adjacent delimiters
            //!
            //! @param line line to be splittet
            //! @param delimiter the value to use as a splitter
            //!
            //! @return the splitted range.
            std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter = ' ');

         } // adjacent

         template< typename R>
         std::string join( R&& range)
         {
            return algorithm::accumulate( range, std::string{});
         }
         

         template< typename R, typename D>
         std::string join( R&& range, D&& delimiter)
         {
            auto current = range::make( range);

            if( ! current)
               return {};

            std::string result = *current;
            while( ++current)
            {
               result += delimiter;
               result += *current;
            }
            return result;
         }

         template< typename R>
         auto trim( R&& range) -> traits::remove_cvref_t< decltype( range)>
         {
            const auto ws = [] (const auto character){ 
               return std::isspace( character, std::locale::classic()); 
            };

            auto last = std::end( range);
            const auto first = std::find_if_not( std::begin( range), last, ws);

            for( ; last != first && ws( *( last -1)); --last)
               ;

            return traits::remove_cvref_t< decltype( range)>{ first, last};
         }


         std::string lower( std::string value);

         std::string upper( std::string value);

         template< typename R>
         bool integer( R&& value)
         {
            view::String view( value);
            
            if( view.empty())
               return false;

            return algorithm::includes( "0123456789", view);
         }

         //! composes a string from several parts, using the stream operator
         template< typename... Parts>
         inline std::string compose( Parts&&... parts)
         {
            std::ostringstream out;
            stream::write( out, std::forward< Parts>( parts)...);
            return out.str();
         }
      } // string

      namespace detail
      {
         auto c_wrapper = []( view::String value, auto&& function, auto... base) noexcept
         {
            char* end = nullptr;
            return function( std::begin( value), &end, base...);
         };

         template< typename R, typename Enable = void>
         struct from_string;

         template< typename T >
         struct from_string< T, std::enable_if_t< std::is_integral< T>::value &&  std::is_signed< T>::value>>
         { 
            static T get( view::String value) { return c_wrapper( value, &::strtol, 10);} 
         };

         template< typename T >
         struct from_string< T, std::enable_if_t< std::is_integral< T>::value &&  ! std::is_signed< T>::value>>
         { 
            static T get( view::String value) { return c_wrapper( value, &::strtoul, 10);} 
         };

         template< typename T >
         struct from_string< T, std::enable_if_t< std::is_floating_point< T>::value>>
         { 
            static T get( view::String value) { return c_wrapper( value, &::strtod);} 
         };

         template<>
         struct from_string< bool, void>
         { 
            static bool get( view::String value) 
            {
               if( value == "true") return true; 
               if( value == "false") return false; 
               return ! ( value == "0");
            } 
         };

         template<>
         struct from_string< std::string, void> 
         { 
            static const std::string& get( const std::string& value) { return value;} 
            static std::string get( view::String value) { return { std::begin( value), std::end( value)};} 
         };



         //inline std::string to_string( std::string value) { return value;}
         inline const std::string& to_string( const std::string& value) { return value;}

         inline std::string to_string( const bool value) { std::ostringstream out; out << std::boolalpha << value; return out.str();}

         template< typename T>
         std::string to_string( const T& value) { std::ostringstream out; out << value; return out.str();}

      } // detail

      template< typename R, typename T>
      decltype( auto) from_string( T&& value)
      {
         return detail::from_string< std::decay_t< R>>::get( value);
      }

      template< typename T>
      decltype( auto) to_string( T&& value)
      {
         return detail::to_string( std::forward< T>( value));
      }


      namespace type
      {
         namespace internal
         {
            std::string name( const std::type_info& type);
         } // internal

         template< typename T>
         std::string name()
         {
            return internal::name( typeid( T));
         }

         template< typename T>
         std::string name( T&& value)
         {
            return internal::name( typeid( value));
         }
      } // type
   } // common
} // casual




