//!
//! casual_utility_environment.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "common/environment.h"
#include "common/exception.h"
#include "common/file.h"

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

			   namespace local
			   {
               namespace
               {
                  std::string& executablePath()
                  {
                     static std::string path;
                     return path;
                  }
               }
            }

            void executable( const std::string& path)
            {
               local::executablePath() = path;
            }

            const std::string& executable()
            {
               return local::executablePath();
            }

            std::string brokerQueue()
            {
               return directory::domain() + "/.casual_broker_queue";
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


			platform::seconds_type getTime()
			{
				return time( 0);
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
         } // domain



		}
	}
}



