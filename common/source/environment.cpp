//!
//! casual_utility_environment.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "common/environment.h"
#include "common/exception.h"
#include "common/file.h"
#include "common/algorithm.h"

#include <cstdlib>


#include <ctime>


namespace casual
{
	namespace common
	{
		namespace environment
		{
			namespace variable
			{
				bool exists( const std::string& name)
				{
					return getenv( name.c_str()) != nullptr;
				}

				std::string get( const std::string& name)
				{
					const char* const result = getenv( name.c_str());

					if( result)
					{
						return result;
					}
					else
					{
						throw exception::EnvironmentVariableNotFound( name);
					}
				}

            std::string get( const std::string& name, const std::string& alternative)
            {
               if( exists( name))
               {
                  return get( name);
               }
               return alternative;
            }

				void set( const std::string& name, const std::string& value)
				{
				   setenv( name.c_str(), value.c_str(), 1);
				}
			}


			namespace directory
			{
			   const std::string& domain()
            {
               static const std::string result = variable::get( "CASUAL_DOMAIN_HOME");
               return result;
            }

            const std::string& temporary()
            {
               static const std::string result = "/tmp";
               return result;
            }

            const std::string& casual()
            {
               static const std::string result = variable::get( "CASUAL_HOME");
               return result;
            }

         }

			namespace file
         {

            std::string brokerQueue()
            {
               return domain::singleton::path() + "/.casual-broker-queue";
            }

            std::string configuration()
            {
               return common::file::find( directory::domain() + "/configuration", std::regex( "domain.(yaml|xml)" ));
            }

            std::string installedConfiguration()
            {
               return common::file::find( directory::casual() + "/configuration", std::regex( "resources.(yaml|xml)" ));
            }

         }


			namespace local
         {
            namespace
            {
               std::string& domainName()
               {
                  static std::string path;
                  return path;
               }
            }
		   }

			namespace domain
         {
            const std::string& name()
            {
               return local::domainName();
            }

            void name( const std::string& value)
            {
               local::domainName() = value;
            }

            namespace singleton
            {
               namespace local
               {
                  namespace
                  {
                     std::string path( std::string path)
                     {
                        common::directory::create( path);
                        return path;
                     }

                  } // <unnamed>
               } // local

               const std::string& path()
               {
                  static const std::string path = local::path( directory::domain() + "/.singleton");
                  return path;
               }

            } // singleton

         } // domain


         std::string string( const std::string& value)
         {
            std::string result;
            auto divide = range::divide_first( value, "$(");

            while( std::get< 1>( divide))
            {

               result.insert( std::end( result), std::begin( std::get< 0>( divide)), std::end( std::get< 0>( divide)));

               auto current = std::get< 1>( divide);
               auto variable = current;

               //
               // Make sure we're past the "$("
               //
               ++variable;
               ++variable;

               auto rest = range::divide_first( variable, ")");

               if( std::get< 1>( rest))
               {
                  const std::string name{ std::begin( std::get< 0>( rest)), std::end( std::get< 0>( rest))};

                  result += environment::variable::get( name);

                  current = std::get< 1>( rest)++;
               }
               else
               {
                  result.insert( std::end( result), std::begin( current), std::end( current));
                  current = std::get< 1>( rest);
               }

               divide = range::divide_first( current, "$(");
            }

            result.insert( std::end( result), std::begin( std::get< 0>( divide)), std::end( std::get< 0>( divide)));

            return result;
         }

		} // environment
	} // common
} // casual


