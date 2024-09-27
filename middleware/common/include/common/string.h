//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "casual/platform.h"
#include "common/string/view.h"
#include "common/traits.h"
#include "common/algorithm.h"
#include "common/stream.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include <string>
#include <locale>
#include <charconv>
#include <regex>
#include <algorithm>
#include <sstream>


namespace casual
{
   namespace common::string
   {
      //! splits a range
      //!
      //! @param line line to be splittet
      //! @param delimiter the value to use as a splitter
      //!
      //! @return the splitted range.
      std::vector< std::string> split( std::string_view line, typename std::string::value_type delimiter = ' ');


      namespace adjacent
      {
         //! splits a range, and ignores adjacent delimiters
         //!
         //! @param line line to be splittet
         //! @param delimiter the value to use as a splitter
         //!
         //! @return the splitted range.
         std::vector< std::string> split( std::string_view line, typename std::string::value_type delimiter = ' ');

      } // adjacent

      template< typename R>
      std::string join( R&& range)
      {
         return algorithm::accumulate( range, std::string{});
      }
      

      template< typename R, typename D>
      std::string join( R&& range, D&& delimiter)
      {
         std::ostringstream out;
         algorithm::for_each_interleave( 
            range, 
            [&out]( auto& value){ out << value;},
            [&out,&delimiter](){ out << delimiter;});

         return std::move( out).str();
      }

      namespace detail
      {
         template< typename R, typename Iter>
         auto create( Iter first, Iter last, traits::priority::tag< 1>) -> decltype( R{ first, last}) { return R{ first, last};}

         template< typename R, typename Iter>
         auto create( Iter first, Iter last, traits::priority::tag< 0>) -> decltype( R( first, std::distance( first, last))) 
         { 
            return R( first, std::distance( first, last));
         }
      } // detail

      template< typename R>
      auto trim( R&& range) -> std::remove_cvref_t< decltype( range)>
      {
         const auto ws = [] ( const auto character)
         { 
            return std::isspace( character, std::locale::classic()); 
         };

         auto last = std::end( range);
         const auto first = std::find_if_not( std::begin( range), last, ws);

         for( ; last != first && ws( *( last -1)); --last)
            ;

         // TODO maintainence: remove dispatch when string_view get ctor with only iterators - C++20
         return detail::create< std::remove_cvref_t< decltype( range)>>( first, last, traits::priority::tag< 1>{});
      }


      std::string lower( std::string value);

      std::string upper( std::string value);

      template< typename R>
      bool integer( R&& value)
      {
         auto view = string::view::make( value);
         
         if( view.empty())
            return false;

         return algorithm::includes( "0123456789", view);
      }

      namespace detail
      {
         inline void from( std::string_view value, bool& result, traits::priority::tag< 2>)
         { 
            if( value == "true") 
               result = true; 
            else if( value == "false") 
               result = false; 
            else 
               result = ! ( value == "0");
         }

         inline void from( std::string_view value, std::string& result, traits::priority::tag< 2>)
         {
            result = value;
         }

         template< typename R>
         auto from( std::string_view value, R& result, traits::priority::tag< 1>) 
            -> decltype( void( std::from_chars( value.data(), value.data() + value.size(), result)))
         { 
            if( std::from_chars( value.data(), value.data() + value.size(), result).ec == std::errc::invalid_argument)
               code::raise::error( code::casual::invalid_argument, "failed to convert from string: ", value);
         }

         //! last resort, use stream operator
         template< typename R>
         auto from( std::string_view value, R& result, traits::priority::tag< 0>) 
            -> decltype( void( std::declval< std::istream&>() >> result))
         { 
            std::istringstream in{ std::string{ value}};
            in >> result;
         }


         //inline std::string to_string( std::string value) { return value;}
         inline const std::string& to( const std::string& value) { return value;}

         inline std::string to( const bool value) 
         {
            std::ostringstream out; 
            out << std::boolalpha << value; 
            return std::move( out).str();
         }

         template< typename T>
         std::string to( const T& value) 
         { 
            std::ostringstream out; 
            out << value;
            return std::move( out).str();
         }

      } // detail


      template< typename R>
      [[nodiscard]] auto from( std::string_view string) 
         -> decltype( void( detail::from( string, std::declval< R&>(), traits::priority::tag< 2>{})), R{})
      {
         R result{};
         detail::from( string, result, traits::priority::tag< 2>{});
         return result;
      }


      template< typename T>
      [[nodiscard]] auto to( T&& value)
         -> decltype( detail::to( std::forward< T>( value)))
      {
         return detail::to( std::forward< T>( value));
      }

      //! Helper to "catch" string-like values and convert to std::string
      struct Argument
      {
         inline Argument( std::string value) : m_value( std::move( value)) {}
         inline Argument( std::string_view value) : Argument( std::string( value)) {}
         inline Argument( const char* value) : Argument( std::string( value)) {}

         operator std::string&& () && { return std::move( m_value);}

      private:
         std::string m_value;
      };

   } // common::string
} // casual




