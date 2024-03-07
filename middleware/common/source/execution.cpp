//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/execution.h"
#include "common/uuid.h"
#include "common/environment.h"
#include "common/exception/capture.h"


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
               auto initialize()
               {
                  if( environment::variable::exists( environment::variable::name::execution::id))
                     return Uuid{ environment::variable::get( environment::variable::name::execution::id)};
                  return uuid::make();
               }

               execution::type& id()
               {
                  static execution::type id{ initialize()};
                  return id;
               }
            } // <unnamed>
         } // local

         void id( const type& id)
         {
            local::id() = id;
         }

         void reset()
         {
            local::id() = execution::type{ uuid::make()};
         }

         const type& id()
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


