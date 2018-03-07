//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/execution.h"
#include "common/uuid.h"


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
               Uuid& id()
               {
                  static Uuid id = common::uuid::make();
                  return id;
               }
            } // <unnamed>
         } // local

         void id( const Uuid& id)
         {
            local::id() = id;
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
