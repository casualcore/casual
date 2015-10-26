//!
//! execution.cpp
//!
//! Created on: Jun 7, 2015
//!     Author: Lazan
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

               std::string& service()
               {
                  static std::string service;
                  return service;
               }

               namespace parent
               {
                  std::string& service()
                  {
                     static std::string service;
                     return service;
                  }
               } // parent

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


         void service( const std::string& service)
         {
            local::service() = service;
         }

         const std::string& service()
         {
            return local::service();
         }

         namespace parent
         {

            void service( const std::string& service)
            {
               local::parent::service() = service;
            }

            const std::string& service()
            {
               return local::parent::service();
            }

         } // parent

      } // execution
   } // common


} // casual
