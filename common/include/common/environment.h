//!
//! casual_utility_environment.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_ENVIRONMENT_H_
#define CASUAL_UTILITY_ENVIRONMENT_H_

#include <string>
#include <sstream>

#include "common/platform.h"

namespace casual
{
	namespace utility
	{
		namespace environment
		{
			namespace variable
			{
				bool exists( const std::string& name);

				std::string get( const std::string& name);

				template< typename T>
				T get( const std::string& name)
				{
					std::istringstream converter( get( name));
					T result;
					converter >> result;
					return result;
				}

			}

			const std::string& getTemporaryPath();

			const std::string& getRootPath();

			std::string getBrokerQueueFileName();

			platform::seconds_type getTime();

			std::string getDomainName();

			std::string getDefaultConfigurationFile();

			void setExecutablePath( const std::string& path);

			const std::string& getExecutablePath();


		}

	}


}


#endif /* CASUAL_UTILITY_ENVIRONMENT_H_ */
