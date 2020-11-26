//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/platform.h"
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
            std::ostringstream out;
            algorithm::for_each_interleave( 
               range, 
               [&out]( auto& value){ out << value;},
               [&out,&delimiter](){ out << delimiter;});

            return std::move( out).str();
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
            return std::move( out).str();
         }
      } // string

      namespace detail
      {
         template< typename F, typename... B>
         auto c_wrapper( view::String value, F&& function, B... base) noexcept
         {
            char* end = nullptr;
            return function( std::begin( value), &end, base...);
         }

         namespace from
         {
            //! last resort, use stream operator
            template< typename R>
            auto string( view::String value, R& result, traits::priority::tag< 0>) 
               -> decltype( void( std::declval< std::istream&>() >> result))
            { 
               std::istringstream in{ value.value()};
               in >> result;
            }

            template< typename R>
            auto string( view::String value, R& result, traits::priority::tag< 1>) 
               -> std::enable_if_t< std::is_integral< R>::value &&  std::is_signed< R>::value>
            { 
               result = c_wrapper( value, &std::strtol, 10);
            }

            template< typename R>
            auto string( view::String value, R& result, traits::priority::tag< 1>) 
               -> std::enable_if_t< std::is_integral< R>::value &&  ! std::is_signed< R>::value>
            { 
               result = c_wrapper( value, &std::strtoul, 10);
            }

            template< typename R>
            auto string( view::String value, R& result, traits::priority::tag< 1>) 
               -> std::enable_if_t< std::is_floating_point< R>::value>
            { 
               result = c_wrapper( value, &std::strtod);
            }

            template< typename R>
            auto string( view::String value, R& result, traits::priority::tag< 2>) 
               -> std::enable_if_t< std::is_same< R, bool>::value>
            { 
               if( value == "true") result = true; 
               else if( value == "false") result = false; 
               else result = ! ( value == "0");
            }

         } // from


         template< typename R, typename Enable = void>
         struct from_string;

         template< typename T >
         struct from_string< T, void>
         { 
            static T get( view::String value) 
            {
               T result;
               from::string( value, result, traits::priority::tag< 2>{});
               return result;
            } 
         };

         //! strings just go through
         template<>
         struct from_string< std::string, void> 
         { 
            static const std::string& get( const std::string& value) { return value;} 
            static std::string get( std::string&& value) { return std::move( value);} 
            static std::string get( view::String value) { return { std::begin( value), std::end( value)};} 
         };




         //inline std::string to_string( std::string value) { return value;}
         inline const std::string& to_string( const std::string& value) { return value;}

         inline std::string to_string( const bool value) 
         {
            std::ostringstream out; 
            out << std::boolalpha << value; 
            return std::move( out).str();
         }


         template< typename T>
         std::string to_string( const T& value) 
         { 
            std::ostringstream out; 
            out << value;
            return std::move( out).str();
         }

      } // detail

      template< typename R, typename T>
      auto from_string( T&& value) 
         -> decltype( detail::from_string< std::decay_t< R>>::get( std::forward< T>( value)))
      {
         return detail::from_string< std::decay_t< R>>::get( std::forward< T>( value));
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




