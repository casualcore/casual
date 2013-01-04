//!
//! server.cpp
//!
//! Created on: Dec 27, 2012
//!     Author: Lazan
//!

#include "sf/server.h"
#include "sf/service_implementation.h"

namespace casual
{
   namespace sf
   {
      namespace server
      {


         std::unique_ptr< service::Interface> Interface::service( TPSVCINFO* serviceInfo)
         {
            return doGetService( serviceInfo);
         }

         Interface::~Interface()
         {

         }

         class Implementation : public Interface
         {

         private:
            std::unique_ptr< service::Interface> doGetService( TPSVCINFO* serviceInfo) override
            {
               // TODO:
               return std::unique_ptr< service::Interface>( new service::implementation::Base{});
            }

         };


         std::unique_ptr< Interface> create( int argc, char **argv)
         {
            return std::unique_ptr< Interface>( new Implementation{});
         }

      } // server
   } // sf
} // casual



