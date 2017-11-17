//!
//! casual
//!

#ifndef CASUAL_UTILITY_ENVIRONMENT_H_
#define CASUAL_UTILITY_ENVIRONMENT_H_

#include "common/platform.h"
#include "common/uuid.h"
#include "common/process.h"

#include <string>
#include <sstream>
#include <mutex>

namespace casual
{
	namespace common
	{
		namespace environment
		{

			namespace variable
			{

			   //!
			   //! Exposes the mutex that is used to get read and writes to environment variables
			   //! thread safe.
			   //!
			   //! There are a few other places that uses and manipulates environment variables, and
			   //! we need those to lock and use the same mutex.
			   //!
			   //! Known places:
			   //!
			   //! - chronology::internal::format  localtime_r uses TZ
			   //! - process::spawn "forwards" the current environment variables to the child process
			   //!
			   //! @return
			   std::mutex& mutex();

			   bool exists( const char* name);
				inline bool exists( const std::string& name) { return exists( name.c_str());}


				namespace detail
            {
				   inline const char* get_name( const char* name) { return name;}
				   inline const char* get_name( const std::string& name) { return name.c_str();}


				   std::string get( const char* name);
				   std::string get( const char* name, std::string alternative);

				   void set( const char* name, const std::string& value);

            } // detail


				//!
				//! @return value of environment variable with @p name
				//! @throws exception::EnvironmentVariableNotFound if not found
				//!
				template< typename String>
				std::string get( String&& name) { return detail::get( detail::get_name( name));}

            //!
            //! @return value of environment variable with @p name or value of alternative if
            //!   variable isn't found
            //!
				template< typename String>
            std::string get( String&& name, std::string alternative) { return detail::get( detail::get_name( name), std::move( alternative));}


            template< typename T, typename String>
            T get( String&& name)
            {
               std::istringstream converter( detail::get( detail::get_name( name)));
               T result;
               converter >> result;
               return result;
            }


            template< typename String, typename T>
            auto get( String&& name, T value) -> std::enable_if_t< std::is_integral< T>::value, T>
            {
               if( exists( name))
                  return get< T>( name);

               return value;
            }


            template< typename String>
            void set( String&& name, const std::string& value) { detail::set( detail::get_name( name), value);}


            template< typename String, typename T>
            void set( String&& name, T&& value)
            {
               std::ostringstream converter;
               converter << value;
               const std::string& string = converter.str();
               set( name, string);
            }

				namespace process
            {
				   common::process::Handle get( const char* name);
				   inline common::process::Handle get( const std::string& name) { return get( name.c_str());}

				   void set( const char* name, const common::process::Handle& process);
				   inline void set( const std::string& name, const common::process::Handle& process) { return set( name.c_str(), process);}

            } // process

				namespace name
            {
				   //!
				   //! @return variable name representing casual home. Where casual is installed
				   //!
				   constexpr auto home() { return "CASUAL_HOME";};

				   namespace domain
               {
				      constexpr auto home() { return "CASUAL_DOMAIN_HOME";}
				      constexpr auto id() { return "CASUAL_DOMAIN_ID";}
				      constexpr auto path() { return "CASUAL_DOMAIN_PATH";}
				      constexpr auto name() { return "CASUAL_DOMAIN_NAME";}
               } // domain

				   namespace log
               {
				      constexpr auto path() { return "CASUAL_LOG_PATH";}
               } // log


               //!
               //! the name of the environment variables that holds ipc queue id:s
               //! @{
				   namespace ipc
               {
				      namespace domain
                  {
				         constexpr auto manager() { return "CASUAL_DOMAIN_MANAGER_PROCESS";}
                  } // domain

				      namespace service
                  {
				         constexpr auto manager() { return "CASUAL_SERVICE_MANAGER_PROCESS";}
                  } // service

				      namespace transaction
                  {
				         constexpr auto manager() { return "CASUAL_TRANSACTION_MANAGER_PROCESS";}
                  } // transaction

				      namespace queue
                  {
				         constexpr auto manager() { return "CASUAL_QUEUE_MANAGER_PROCESS";}
                  } // queue

				      namespace gateway
                  {
				         constexpr auto manager() { return "CASUAL_GATEWAY_MANAGER_PROCESS";}
                  } // gateway
               } // ipc
				   //! @}

               namespace terminal
               {
                  constexpr auto precision() { return "CASUAL_TERMINAL_PRECISION";}
               } // log


            } // name
			} // variable

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

			namespace log
         {
			   const std::string& path();
         } // log

			namespace domain
         {

			   namespace singleton
            {
			      const std::string& path();

               const std::string& file();

            } // singleton

         } // domain


			//!
			//! Parses value for environment variables with format @p ${<variable>}
			//! and tries to find and replace the variable from environment.
			//!
			//! @return possible altered string with regards to environment variables
			//!
			std::string string( const std::string& value);


		} // environment
	} // common
} // casual


#endif /* CASUAL_UTILITY_ENVIRONMENT_H_ */
