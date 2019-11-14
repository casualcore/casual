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
#include "common/exception/system.h"


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
               inline const char* get_name( const std::string& name) { return name.c_str();}


               std::string get( const char* name);
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

            //! @return value of environment variable with @p name or value of alternative if
            //!   variable isn't found
            template< typename String>
            std::string get( String&& name, std::string alternative) { return detail::get( detail::get_name( name), std::move( alternative));}


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
                  throw exception::system::invalid::Argument{ string::compose( "failed to convert value of environment variable: ", name)};
               }
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
               const std::string& string = common::to_string( value);;
               set( name, string);
            }

            template< typename String>
            void unset( String&& name) { detail::unset( detail::get_name( name));}

            namespace process
            {
               common::process::Handle get( const char* name);
               inline common::process::Handle get( const std::string& name) { return get( name.c_str());}

               void set( const char* name, const common::process::Handle& process);
               inline void set( const std::string& name, const common::process::Handle& process) { return set( name.c_str(), process);}

            } // process

            namespace name
            {
               //! @return variable name representing casual home. Where casual is installed
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

               
               namespace ipc
               {
                  //! where to hold transient files, such as named-pipes.
                  constexpr auto directory() { return "CASUAL_IPC_DIRECTORY";}
               } // transient
               

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
                  constexpr auto color() { return "CASUAL_TERMINAL_COLOR";}
                  constexpr auto header() { return "CASUAL_TERMINAL_HEADER";}
                  constexpr auto porcelain() { return "CASUAL_TERMINAL_PORCELAIN";}
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

         //! Parses value for environment variables with format @p ${<variable>}
         //! and tries to find and replace the variable from environment.
         //!
         //! @return possible altered string with regards to environment variables
         //! @{
         std::string string( const std::string& value);
         std::string string( std::string&& value);
         //! @}

         //! resets "all" paths to directories and files, based on what 
         //! environment variables are present.
         void reset();

      } // environment
   } // common
} // casual



