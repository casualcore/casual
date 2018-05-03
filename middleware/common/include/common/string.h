//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_STRING_H_
#define CASUAL_COMMON_STRING_H_

#include "common/platform.h"
#include "common/traits.h"
#include "common/algorithm.h"

#include <string>
#include <locale>

#include <regex>
#include <algorithm>
#include <sstream>

namespace casual
{

	namespace common
	{
		namespace string
		{

			//!
			//! splits a range
			//!
			//! @param line line to be splittet
			//! @param delimiter the value to use as a splitter
			//!
			//! @return the splitted range.
			//!
			std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter = ' ');


			namespace adjacent
         {
	         //!
	         //! splits a range, and ignores adjacent delimiters
	         //!
	         //! @param line line to be splittet
	         //! @param delimiter the value to use as a splitter
	         //!
	         //! @return the splitted range.
	         //!
	         std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter = ' ');

         } // adjacent


			/*
			 * regex not implemented in gcc 4.7.2...
			inline std::vector< std::string> split( const std::string& line, const std::regex& regexp)
         {
			   std::vector< std::string> result;

			   std::copy( std::sregex_token_iterator( std::begin( line), std::end( line), regexp, -1),
			         std::sregex_token_iterator(),
			         std::back_inserter( result));

			   return result;
         }

			inline std::vector< std::string> split( const std::string& line, const std::string& regexp)
         {
			   return split( line, std::regex( regexp));
         }
         */


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

			std::string trim( const std::string& value);

			template< typename R>
         auto trim( R&& range) -> common::traits::concrete::type_t< decltype( range)>
         {
            const auto ws = [] (const auto character){ 
               return std::isspace( character, std::locale::classic()); 
            };

            auto last = std::end( range);
            const auto first = std::find_if_not( std::begin( range), last, ws);

            for( ; last != first && ws( *( last -1)); --last)
               ;

            return common::traits::concrete::type_t< decltype( range)>{ first, last};
         }


			std::string lower( std::string value);

         std::string upper( std::string value);

         template< typename R>
         bool integer( R&& value)
         {
            auto range = range::make( value);
            
            if( range.empty())
               return false;

            return algorithm::includes( "0123456789", range);
         }

/*
			template< typename T>
			auto digits( T value) -> std::enable_if_t< std::is_integral< T>::value, platform::size::type>
			{
			   platform::size::type result{ 1};

			   while( value /= 10)
			   {
			      ++result;
			   }
			   return result;
			}
*/
         namespace detail
         {
            template< typename T>
            inline void write( std::ostream& out, const T& value) 
            {
               out << value;
            }

            template< typename T>
            inline void write( std::ostream& out, const std::vector< T>& value) 
            {
               out << range::make( value);
            }

            inline void composer( std::ostream& out) {}

            template< typename Part, typename... Parts>
            inline void composer( std::ostream& out, Part&& part, Parts&&... parts)
            {
               write( out, part);
               composer( out, std::forward< Parts>( parts)...);
            }
         } // detail

         //!
         //! composes a string from several parts, using the stream operator
         //!
         template< typename... Parts>
         inline std::string compose( Parts&&... parts)
         {
            std::ostringstream out;
            detail::composer( out, std::forward< Parts>( parts)...);
            return out.str();
         }
		} // string

		namespace internal
      {
		   template< typename R, typename Enable = void>
		   struct from_string;

         template< typename T >
		   struct from_string< T, std::enable_if_t< std::is_integral< T>::value &&  std::is_signed< T>::value>>
         { 
            static T get( const std::string& value) { return std::stol( value);} 
         };

         template< typename T >
		   struct from_string< T, std::enable_if_t< std::is_integral< T>::value &&  ! std::is_signed< T>::value>>
         { 
            static T get( const std::string& value) { return std::stoul( value);} 
         };

         template< typename T >
		   struct from_string< T, std::enable_if_t< std::is_floating_point< T>::value>>
         { 
            static T get( const std::string& value) { return std::stod( value);} 
         };

         template<>
		   struct from_string< bool, void>
         { 
            static bool get( const std::string& value) 
            {
               if( value == "true") return true; 
               if( value == "false") return false; 
               return std::stoi( value);
            } 
         };

		   template<>
		   struct from_string< std::string, void> 
         { 
            static const std::string& get( const std::string& value) { return value;} 
         };



         //inline std::string to_string( std::string value) { return value;}
         inline const std::string& to_string( const std::string& value) { return value;}

         inline std::string to_string( const bool value) { std::ostringstream out; out << std::boolalpha << value; return out.str();}

         // TODO: Why do we specialize 'long' ?
         //inline std::string to_string( const long value) { return std::to_string( value);}

         template< typename T>
         std::string to_string( const T& value) { std::ostringstream out; out << value; return out.str();}



      } // internal

		template< typename R>
		decltype( auto) from_string( const std::string& value)
		{
		   return internal::from_string< std::decay_t< R>>::get( value);
		}

		template< typename T>
		decltype( auto) to_string( T&& value)
		{
		   return internal::to_string( std::forward< T>( value));
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



#endif /* CASUAL_COMMON_STRING_H_ */
