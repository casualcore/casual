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

				//!
				//! @return value of invironment variable with @p name
				//! @throws exception::EnvironmentVariableNotFound if not found
				//!
				std::string get( const std::string& name);

            //!
            //! @return value of environment variable with @p name or value of alternative if
            //!   variable isn't found
            //!
            std::string get( const std::string& name, std::string alternative);

				template< typename T>
				T get( const std::string& name)
				{
					std::istringstream converter( get( name));
					T result;
					converter >> result;
					return result;
				}


				void set( const std::string& name, const std::string& value);

				template< typename T>
				void set( const std::string& name, T&& value)
				{
				   std::ostringstream converter;
				   converter << value;
				   const std::string& string = converter.str();
				   set( name, string);
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




			//platform::seconds_type getTime();

			namespace domain
         {
			   //!
			   //! @return the name of the casual domain.
			   //!
			   const std::string& name();

			   void name( const std::string& value);

			   namespace singleton
            {
			      const std::string& path();

            } // singleton

         } // domain


			//!
			//! Parses value for environment variables with format @p $(<variable>)
			//! and tries to find and replace the variable from environment.
			//!
			//! @return possible altered string with regards to environment variables
			//!
			std::string string( const std::string& value);


		} // environment
	} // common
} // casual


#endif /* CASUAL_UTILITY_ENVIRONMENT_H_ */
