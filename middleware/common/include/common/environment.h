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

            bool exists( std::string_view name);


            namespace native
            {
               //! @returns all current environment variables in the format [name=value]
               std::vector< environment::Variable> current();
            } // native


            //! @return value of environment variable with @p name
            //! @throws exception::EnvironmentVariableNotFound if not found
            std::string get( std::string_view name);


            template< typename T>
            auto get( std::string_view name)
               -> decltype( common::string::from< T>( std::string_view{}))
            {
               auto value = variable::get( name);
               try 
               {
                  return common::string::from< T>( std::move( value));
               }
               catch( ...)
               {
                  code::raise::error( code::casual::invalid_argument, "failed to convert value of environment variable: ", name);
               }
            }
            
            namespace detail
            {

               template< typename A>
               auto get( std::string_view name, A alternative, traits::priority::tag< 2>)
                  -> std::enable_if_t< std::is_convertible_v< A, std::string>, std::string>
               {
                  if( variable::exists( name))
                     return variable::get( name);

                  return alternative;
               }

               // callable alternative
               template< typename A>
               auto get( std::string_view name, A&& alternative, traits::priority::tag< 1>)
                  -> decltype( common::string::from< decltype( alternative())>( variable::get( name)))
               {
                  if( variable::exists( name))
                     return common::string::from< decltype( alternative())>( variable::get( name));

                  return alternative();
               }

               template< typename A>
               auto get( std::string_view name, A alternative, traits::priority::tag< 0>)
                  -> decltype( common::string::from< A>( variable::get( name)))
               {
                  if( variable::exists( name))
                     return common::string::from< A>( variable::get( name));

                  return alternative;
               }

               void set( std::string_view name, std::string_view value);

               template< typename T>
               auto set( std::string_view name, T&& value, traits::priority::tag< 2>) 
                  -> decltype( set( name, std::forward< T>( value)))
               {
                  set( name, std::forward< T>( value));
               }

               //! for values that is not "string", but have a string() member
               // (std::filesystem::path)
               template< typename T>
               auto set( std::string_view name, T&& value, traits::priority::tag< 1>) 
                  -> decltype( set( name, std::forward< T>( value).string()))
               {
                  set( name, std::forward< T>( value).string());
               }

               template< typename T>
               auto set( std::string_view name, T&& value, traits::priority::tag< 0>) 
                  -> decltype( void( common::string::to( std::forward< T>( value))))
               {
                  auto&& string = common::string::to( std::forward< T>( value));
                  set( name, string);
               }

            } // detail


            //! @return value of environment variable with @p name or `alternative` if
            //!   variable isn't found
            template< typename A>
            auto get( std::string_view name, A&& alternative) 
               -> decltype( detail::get( name, std::forward< A>( alternative), traits::priority::tag< 2>{}))
            {
               return detail::get( name, std::forward< A>( alternative), traits::priority::tag< 2>{});
            }


            template< typename T>
            auto set( std::string_view name, T&& value) 
               -> decltype( detail::set( name, std::forward< T>( value), traits::priority::tag< 1>{}))
            {
               detail::set( name, std::forward< T>( value), traits::priority::tag< 2>{});
            }

            void unset( std::string_view name);

            //! consumes an environment variable (get and then unset)
            inline std::string consume( std::string_view name, std::string alternative) 
            { 
               if( ! exists( name))
                  return alternative;

               auto result = variable::get( name);
               unset( name);
               return result;
            }

            namespace process
            {
               common::process::Handle get( std::string_view name);
               void set( std::string_view, const common::process::Handle& process);

            } // process

            namespace name
            {
               using namespace std::string_view_literals;

               namespace unittest
               {
                  //! if set, we're in a unittest context and can act differently
                  //! TODO we might want to have a #define from the compiler
                  constexpr auto context = "CASUAL_UNITTEST_CONTEXT"sv;
               } // unittest
               
               namespace system
               {
                  constexpr auto configuration = "CASUAL_SYSTEM_CONFIGURATION_GLOB"sv;
               } // system

               namespace resource
               {
                  //! @deprecated use name::system::configuration
                  constexpr auto configuration = "CASUAL_RESOURCE_CONFIGURATION_FILE"sv;
               } // resource

               namespace log
               {
                  constexpr auto pattern = "CASUAL_LOG"sv;
                  constexpr auto path = "CASUAL_LOG_PATH"sv;

                  namespace parameter
                  {
                     constexpr auto format = "CASUAL_LOG_PARAMETER_FORMAT"sv;
                  } // parameter

               } // log

               
               namespace directory
               {
                  //! variable name representing casual home. Where casual is installed
                  constexpr auto install = "CASUAL_HOME"sv;

                  constexpr auto domain = "CASUAL_DOMAIN_HOME"sv;

                  //! where to store ipc files
                  constexpr auto ipc = "CASUAL_IPC_DIRECTORY"sv;

                  //! where to store transaction (TLOG) database files
                  constexpr auto transaction = "CASUAL_TRANSACTION_DIRECTORY"sv;

                  //! where to store queue database files
                  constexpr auto queue = "CASUAL_QUEUE_DIRECTORY"sv;

                  constexpr auto transient = "CASUAL_TRANSIENT_DIRECTORY"sv;
                  constexpr auto persistent = "CASUAL_PERSISTENT_DIRECTORY"sv;
               } // directory
               

               //! the name of the environment variables that holds ipc queue id:s
               //! @{
               namespace ipc
               {
                  namespace domain
                  {
                     constexpr auto manager = "CASUAL_DOMAIN_MANAGER_PROCESS"sv;
                  } // domain

                  namespace service
                  {
                     constexpr auto manager = "CASUAL_SERVICE_MANAGER_PROCESS"sv;
                  } // service

                  namespace transaction
                  {
                     constexpr auto manager = "CASUAL_TRANSACTION_MANAGER_PROCESS"sv;
                  } // transaction

                  namespace queue
                  {
                     constexpr auto manager = "CASUAL_QUEUE_MANAGER_PROCESS"sv;
                  } // queue

                  namespace gateway
                  {
                     constexpr auto manager = "CASUAL_GATEWAY_MANAGER_PROCESS"sv;
                  } // gateway
               } // ipc
               //! @}

               namespace terminal
               {
                  constexpr auto precision = "CASUAL_TERMINAL_PRECISION"sv;
                  constexpr auto color = "CASUAL_TERMINAL_COLOR"sv;
                  constexpr auto header = "CASUAL_TERMINAL_HEADER"sv;
                  constexpr auto porcelain = "CASUAL_TERMINAL_PORCELAIN"sv;
                  constexpr auto block = "CASUAL_TERMINAL_BLOCK"sv;
                  constexpr auto verbose = "CASUAL_TERMINAL_VERBOSE"sv;
                  constexpr auto editor = "CASUAL_TERMINAL_EDITOR"sv;
               } // log

            } // name
         } // variable

         namespace directory
         {
            //! @return default temp directory
            std::filesystem::path temporary();

            //! @return Home of current domain
            const std::filesystem::path& domain();

            //! @return Where casual is installed
            const std::filesystem::path& install();

            //! where to hold ipc files
            const std::filesystem::path& ipc();

            //! where to hold queue database files
            const std::filesystem::path& queue();

            //! where to hold transaction database files
            const std::filesystem::path& transaction();
         } // directory

         namespace log
         {
            const std::filesystem::path& path();
         } // log

         namespace domain
         {
            namespace singleton
            {
               const std::filesystem::path& file();
            } // singleton
         } // domain


         //! resets "all" paths to directories and files, based on what 
         //! environment variables are present.
         void reset();

      } // environment
   } // common
} // casual



