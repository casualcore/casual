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

#include "common/serialize/json.h"

#include <string>
#include <mutex>

namespace casual
{   
   namespace common::environment
   {
      namespace variable
      {
         namespace detail
         {
            using std::to_string;

            template< typename T>
            concept has_to_string = requires( T value)
            {
               { to_string( value)} -> std::same_as< std::string>;
            };

            template< typename T>
            concept environment_serializable = requires
            {
               typename T::environment_variable_enable;
            };

            template< typename T>
            concept string_serialize = has_to_string< T> || environment_serializable< T>;

            template< string_serialize T>
            auto invoke_string_serialize( const T& value)
            {
               if constexpr( has_to_string< T>)
                  return to_string( value);
               if constexpr( environment_serializable< T>)
               {
                  auto writer = serialize::json::writer();
                  writer << value;
                  return writer.consume< std::string>();
               }
            }

            template< typename T>
            concept string_deserialize = environment_serializable< T> || std::convertible_to< long, T> || std::convertible_to< std::string, T>;

            template< string_deserialize T>
            T invoke_string_deserialize( std::string value)
            {
               if constexpr( environment_serializable< T>)
               {
                  auto reader = serialize::json::relaxed::reader( value);
                  T value{};
                  reader >> value;
                  return value;
               }
               if constexpr( std::convertible_to< std::string, T>)
                  return value;
               if constexpr( std::convertible_to< long, T>)
                  return std::stol( value);
            }

            template<>
            bool invoke_string_deserialize< bool>( std::string value);
            
         } // detail



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

         //! @returns true if the variable `name` exists
         bool exists( std::string_view name);
         
         //! @returns only the immutable system environment variables
         std::vector< environment::Variable> system();

         //! @returns all current environment variables ( mutable prioritized and immutable system) in the format [name=value]
         std::vector< environment::Variable> current();


         //! @return value of environment variable with `name`, if found.
         //! @note will first try to find `name` in _mutable prioritized_, otherwise in _immutable system_. 
         std::optional< std::string> get( std::string_view name);

         //! @return optional of type T that is converted from the string value, if found
         //! @note will first try to find `name` in _mutable prioritized_, otherwise in _immutable system_.
         template< detail::string_deserialize T>
         std::optional< T> get( std::string_view name)
         {
            if( auto value = variable::get( name))
               return std::optional< T>{ detail::invoke_string_deserialize< T>( std::move( *value))};
            
            return {};
         }

         //! adds or update variable `name` to `value` (this does not mutate the system environment)
         void set( std::string_view name, std::string_view value);

         //! adds or update variable `name` to `value` (this does not mutate the system environment)
         template< detail::string_serialize T>
         void set( std::string_view name, const T& value)
         {
            set( name, detail::invoke_string_serialize( value));
         }

         //! consumes an environment variable (get and then unset)
         std::optional< std::string> consume( std::string_view name);

         void unset( std::string_view name); 


         namespace name
         {
            using namespace std::string_view_literals;


            //! stuff that is used for internal technical "configuration"
            namespace internal
            {
               namespace unittest
               {
                  //! if set, we're in a unittest context and can act differently
                  //! TODO we might want to have a #define from the compiler
                  constexpr auto context = "CASUAL_INTERNAL_UNITTEST_CONTEXT"sv;
               } // unittest

               namespace discovery::accumulate
               {
                  constexpr auto requests = "CASUAL_INTERNAL_DISCOVERY_ACCUMULATE_REQUESTS"sv;
                  constexpr auto timeout =  "CASUAL_INTERNAL_DISCOVERY_ACCUMULATE_TIMEOUT"sv;
               } // discovery::accumulate

               namespace gateway::protocol
               {
                  constexpr auto version = "CASUAL_INTERNAL_GATEWAY_PROTOCOL_VERSION"sv;
                  
               } // gateway::protocol
               
            } // internal

            
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

            namespace execution
            {
               constexpr auto id = "CASUAL_EXECUTION_ID"sv;
            } // execution
            

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

   } // common::environment
} // casual



