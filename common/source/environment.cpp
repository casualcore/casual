//!
//! casual_utility_environment.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "common/environment.h"
#include "common/exception.h"
#include "common/file.h"

#include <stdlib.h>


#include <time.h>


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
					return getenv( name.c_str()) != 0;
				}

				std::string get( const std::string& name)
				{
					char* result = getenv( name.c_str());

					if( result)
					{
						return result;
					}
					else
					{
						throw exception::EnvironmentVariableNotFound( name);
					}

				}
			}

			const std::string& getRootPath()
			{
				static const std::string result = variable::get( "CASUAL_ROOT");
				return result;
			}

			std::string getBrokerQueueFileName()
			{
				return getRootPath() + "/.casual_broker_queue";
			}

			const std::string& getTemporaryPath()
			{
				static const std::string result = "/tmp";
				return result;
			}


			platform::seconds_type getTime()
			{
				return time( 0);
			}

			std::string getDomainName()
			{
				//
				// TODO: Maybe store the domainname in broker-queue-file?
				return "domain-1";
			}

			std::string getDefaultConfigurationFile()
			{
			   return common::file::find( getRootPath(), std::regex( "casual_config.(yaml|xml)" ));
			}

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

			void setExecutablePath( const std::string& path)
			{
			   local::executablePath() = path;
			}

			const std::string& getExecutablePath()
			{
			   return local::executablePath();
			}

		}
	}
}



