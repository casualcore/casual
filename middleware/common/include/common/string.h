//!
//! casual_utility_string.h
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_STRING_H_
#define CASUAL_UTILITY_STRING_H_

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
			inline std::string fromCString( char* string)
			{
				return string == 0 ? "" : string;
			}

			inline std::string fromCString( const char* string)
			{
				return string == 0 ? "" : string;
			}

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


			std::string join( const std::vector< std::string>& strings);
         std::string join( const std::vector< std::string>& strings, const std::string& delimiter);

         template< typename T>
         std::string join( T&& range, const std::string& delimiter)
         {
            std::string result;

            auto current = std::begin( range);

            for( ; current != std::end( range); ++current)
            {
               if( current == std::begin( range))
               {
                  result = static_cast< const std::string&>( *current);
               }
               else
               {
                  result += delimiter + static_cast< const std::string&>( *current);
               }
            }
            return result;
         }

			std::string trim( const std::string& value);

			std::string lower( std::string value);

         std::string upper( std::string value);

			bool integer( const std::string& value);


			template< typename T>
			typename std::enable_if< std::is_integral< T>::value, std::size_t>::type
			digits( T value)
			{
			   std::size_t result{ 1};

			   while( value /= 10)
			   {
			      ++result;
			   }
			   return result;
			}

		} // string

		namespace internal
      {
		   template< typename R>
		   struct from_string;

		   template<>
		   struct from_string< std::string> { static const std::string& get( const std::string& value) { return value;} };

		   template<>
		   struct from_string< int> { static int get( const std::string& value) { return std::stoi( value);} };

		   template<>
		   struct from_string< long> { static long get( const std::string& value) { return std::stol( value);} };



         //inline std::string to_string( std::string value) { return value;}
         inline const std::string& to_string( const std::string& value) { return value;}

         inline std::string to_string( const bool value) { std::ostringstream out; out << std::boolalpha << value; return out.str();}

         // TODO: Why do we specialize 'long' ?
         //inline std::string to_string( const long value) { return std::to_string( value);}

         template< typename T>
         std::string to_string( T& value) { std::ostringstream out; out << value; return out.str();}



      } // internal

		template< typename R>
		R from_string( const std::string& value)
		{
		   return internal::from_string< typename std::decay< R>::type>::get( value);
		}

		template< typename T>
		auto to_string( T&& value) -> decltype( internal::to_string( std::forward< T>( value)))
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



#endif /* CASUAL_UTILITY_STRING_H_ */
