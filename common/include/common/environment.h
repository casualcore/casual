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
	namespace common
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

			namespace directory
			{
			   //!
			   //! @return default temp directory
			   //!
			   const std::string& temporary();

			   //!
			   //! @return Home of current domain
			   //!
			   const std::string& domain();

			   //!
			   //! @return Where casual is installed
			   //!
			   const std::string& casual();
			}


			namespace file
			{

			   //void executable( const std::string& path);

			   //const std::string& executable();

			   std::string brokerQueue();

			   //!
			   //! @return domain configuration file path
			   //!
			   //! TODO: some of these should be in casual::domain
			   std::string configuration();

			   // TODO: change name
			   std::string installedConfiguration();
			}




			platform::seconds_type getTime();

			namespace domain
         {
			   //!
			   //! @return the name of the casual domain.
			   //!
			   const std::string& name();

			   void name( const std::string& value);

         } // domain


		}

	}


}


#endif /* CASUAL_UTILITY_ENVIRONMENT_H_ */
