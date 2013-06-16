//!
//! casual_utility_string.h
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_STRING_H_
#define CASUAL_UTILITY_STRING_H_

#include <string>

#include <regex>

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
			//! @param adjacent if true adjacent found delimiters are ignored
			//!
			//! @return the splitted range.
			//!
			inline std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter = ' ',  bool adjacent = true)
         {
			   typedef typename std::string::value_type value_type;

            std::vector< std::string> result;

            auto found = std::begin( line);
            auto current = std::begin( line);

            do
            {
               found = std::find( current,  std::end( line), delimiter);

               result.emplace_back( current, found);

               if( adjacent)
               {
                  current = std::find_if( found, std::end( line), [=]( value_type value) { return value != delimiter;});
               }
               else
               {
                  current = found + 1;
               }

            }while( found != std::end( line));

            return result;
         }

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

		}


	}

}



#endif /* CASUAL_UTILITY_STRING_H_ */
