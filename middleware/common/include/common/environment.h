//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/environment/variable.h"
#include "casual/platform.h"
#include "common/uuid.h"
#include "common/process.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"


#include <string>
#include <mutex>

namespace casual
{
   namespace common
   {
      namespace environment
      {
         namespace variable
         {
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
               inline view::String get_name( view::String name) { return name;}
               inline const char* get_name( const std::string& name) { return name.c_str();}


               std::string get( const char* name);
               std::string get( view::String name);
               std::string get( const char* name, std::string alternative);

               void set( const char* name, const std::string& value);
               void unset( const char* name);

            } // detail

            namespace native
            {
               //! @returns all current environment variables in the format [name=value]
               std::vector< environment::Variable> current();
            } // native


            //! @return value of environment variable with @p name
            //! @throws exception::EnvironmentVariableNotFound if not found
            template< typename String>
            std::string get( String&& name) { return detail::get( detail::get_name( name));}


            template< typename T, typename String>
            T get( String&& name)
            {
               auto value = detail::get( detail::get_name( name));
               try 
               {
                  return common::from_string< T>( std::move( value));
               }
               catch( ...)
               {
                  code::raise::error( code::casual::invalid_argument, "failed to convert value of environment variable: ", name);
               }
            }
            
            namespace detail
            {

               template< typename S, typename A>
               auto alternative( S&& name, A&& alternative, traits::priority::tag< 2>)
                  -> decltype( detail::get( detail::get_name( name), std::move( alternative)))
               {
                  return detail::get( detail::get_name( name), std::move( alternative));
               }

               // callable alternative
               template< typename S, typename A>
               auto alternative( S&& name, A&& alternative, traits::priority::tag< 1>)
                  -> decltype( common::from_string< decltype( alternative())>( std::string{}))
               {
                  if( variable::exists( name))
                     return common::from_string< decltype( alternative())>( variable::get( name));

                  return alternative();
               }

               template< typename S, typename A>
               auto alternative( S&& name, A&& alternative, traits::priority::tag< 0>)
                  -> decltype( common::from_string< A>( std::string{}))
               {
                  if( variable::exists( name))
                     return common::from_string< A>( variable::get( name));

                  return alternative;
               }

            } // detail

            //! @return value of environment variable with @p name or `alternative` if
            //!   variable isn't found
            template< typename String, typename A>
            auto get( String&& name, A&& alternative) 
               -> decltype( detail::alternative( std::forward< String>( name), std::forward< A>( alternative), traits::priority::tag< 2>{}))
            {
               return detail::alternative( std::forward< String>( name), std::forward< A>( alternative), traits::priority::tag< 2>{});
            }


            template< typename String>
            void set( String&& name, const std::string& value) { detail::set( detail::get_name( name), value);}


            template< typename String, typename T>
            void set( String&& name, T&& value)
            {
               const std::string& string = common::to_string( value);;
               set( name, string);
            }

            template< typename String>
            void unset( String&& name) { detail::unset( detail::get_name( name));}

            //! consumes an environment variable (get and then unset)
            template< typename String>
            std::string consume( String&& name, std::string alternative) 
            { 
               if( ! exists( name))
                  return alternative;

               auto result = detail::get( detail::get_name( name));
               unset( name);
               return result;
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
               //! variable name representing casual home. Where casual is installed
               constexpr auto home = "CASUAL_HOME";

               namespace domain
               {
                  constexpr auto home = "CASUAL_DOMAIN_HOME";
                  constexpr auto id = "CASUAL_DOMAIN_ID";
                  constexpr auto name = "CASUAL_DOMAIN_NAME";
               } // domain

               namespace resource
               {
                  constexpr auto configuration = "CASUAL_RESOURCE_CONFIGURATION_FILE";
               } // resource

               namespace log
               {
                  constexpr auto pattern = "CASUAL_LOG";
                  constexpr auto path = "CASUAL_LOG_PATH";

                  namespace parameter
                  {
                     constexpr auto format = "CASUAL_LOG_PARAMETER_FORMAT";
                  } // parameter

               } // log

               
               namespace ipc
               {
                  //! where to hold transient files, such as named-pipes.
                  constexpr auto directory = "CASUAL_IPC_DIRECTORY";
               } // transient
               

               //! the name of the environment variables that holds ipc queue id:s
               //! @{
               namespace ipc
               {
                  namespace domain
                  {
                     constexpr auto manager = "CASUAL_DOMAIN_MANAGER_PROCESS";
                  } // domain

                  namespace service
                  {
                     constexpr auto manager = "CASUAL_SERVICE_MANAGER_PROCESS";
                  } // service

                  namespace transaction
                  {
                     constexpr auto manager = "CASUAL_TRANSACTION_MANAGER_PROCESS";
                  } // transaction

                  namespace queue
                  {
                     constexpr auto manager = "CASUAL_QUEUE_MANAGER_PROCESS";
                  } // queue

                  namespace gateway
                  {
                     constexpr auto manager = "CASUAL_GATEWAY_MANAGER_PROCESS";
                  } // gateway
               } // ipc
               //! @}

               namespace terminal
               {
                  constexpr auto precision = "CASUAL_TERMINAL_PRECISION";
                  constexpr auto color = "CASUAL_TERMINAL_COLOR";
                  constexpr auto header = "CASUAL_TERMINAL_HEADER";
                  constexpr auto porcelain = "CASUAL_TERMINAL_PORCELAIN";
                  constexpr auto block = "CASUAL_TERMINAL_BLOCK";
                  constexpr auto verbose = "CASUAL_TERMINAL_VERBOSE";
               } // log

            } // name
         } // variable

         namespace directory
         {
            //! @return default temp directory
            const std::string& temporary();

            //! @return Home of current domain
            const std::string& domain();

            //! @return Where casual is installed
            const std::string& casual();
         }

         namespace log
         {
            const std::string& path();
         } // log

         namespace ipc
         {
            //! where to hold ipc files, such as named-pipes.
            //! default: /<tmp>/casual/ipc
            const std::string& directory();
         } // log

         namespace domain
         {
            namespace singleton
            {
               const std::string& file();
            } // singleton
         } // domain


         //! resets "all" paths to directories and files, based on what 
         //! environment variables are present.
         void reset();

      } // environment
   } // common
} // casual



