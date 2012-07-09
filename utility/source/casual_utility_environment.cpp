//!
//! casual_utility_environment.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "casual_utility_environment.h"
#include "casual_exception.h"

#include <stdlib.h>


#include <time.h>


namespace casual
{
	namespace utility
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

		}
	}
}



