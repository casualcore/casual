//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/execution.h"
#include "common/uuid.h"
#include "common/environment.h"
#include "common/exception/handle.h"


namespace casual
{
   namespace common
   {
      namespace execution
      {
         namespace local
         {
            namespace
            {
               constexpr auto environment = "CASUAL_EXECUTION_ID";

               auto get()
               {
                  auto id = uuid::make();
                  environment::variable::set( environment, uuid::string( id));
                  return id;
               }

               auto initialize()
               {
                  if( environment::variable::exists( environment))
                  {
                     try 
                     {
                        return Uuid{ environment::variable::get( environment)};
                     }
                     catch( ...)
                     {
                        exception::handle();
                     }
                  }
                  return get();
               }

               Uuid& id()
               {
                  static Uuid id = initialize();
                  return id;
               }
            } // <unnamed>
         } // local

         void id( const Uuid& id)
         {
            local::id() = id;
         }

         void reset()
         {
            local::id() = local::get();
         }

         const Uuid& id()
         {
            return local::id();
         }

         namespace service
         {
            namespace local
            {
               namespace
               {
                  std::string& name()
                  {
                     static std::string name;
                     return name;
                  }
               } // <unnamed>
            } // local


            void name( const std::string& service)
            {
               local::name() = service;
            }

            const std::string& name()
            {
               return local::name();
            }

            void clear()
            {
               local::name().clear();
            }


            namespace parent
            {
               namespace local
               {
                  namespace
                  {
                     std::string& name()
                     {
                        static std::string name;
                        return name;
                     }
                  } // <unnamed>
               } // local

               void name( const std::string& service)
               {
                  local::name() = service;
               }

               const std::string& name()
               {
                  return local::name();
               }

               void clear()
               {
                  local::name().clear();
               }

            } // parent
         } // service

      } // execution
   } // common


} // casual
