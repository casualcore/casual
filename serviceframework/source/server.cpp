//!
//! server.cpp
//!
//! Created on: Dec 27, 2012
//!     Author: Lazan
//!

#include "sf/server.h"

namespace casual
{
   namespace sf
   {
      namespace server
      {


         std::unique_ptr< service::Interface> Interface::service()
         {
            return doService();
         }

         Interface::~Interface()
         {

         }

         class Implementation : public Interface
         {

         private:
            std::unique_ptr< service::Interface> doService()
            {
               // TODO:
               return std::unique_ptr< service::Interface>( new service::Interface{});
            }

         };


         std::unique_ptr< Interface> create( int argc, char **argv)
         {
            return std::unique_ptr< Interface>( new Implementation{});
         }

      } // server
   } // sf
} // casual



