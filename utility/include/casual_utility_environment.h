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

		}

	}


}


#endif /* CASUAL_UTILITY_ENVIRONMENT_H_ */
