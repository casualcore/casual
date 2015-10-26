//!
//! server.cpp
//!
//! Created on: Dec 27, 2012
//!     Author: Lazan
//!

#include "sf/server.h"


#include "common/log.h"

namespace casual
{
   namespace sf
   {
      namespace server
      {


         service::IO Interface::createService( TPSVCINFO* serviceInfo)
         {
            return service::IO{ doCreateService( serviceInfo)};
         }

         Interface::~Interface()
         {

         }

         void Interface::handleException( TPSVCINFO* serviceInfo, service::reply::State& reply)
         {
            doHandleException( serviceInfo, reply);
         }

         class Implementation : public Interface
         {

         private:
            std::unique_ptr< service::Interface> doCreateService( TPSVCINFO* serviceInfo) override
            {
               return sf::service::Factory::instance().create( serviceInfo);
            }

            void doHandleException( TPSVCINFO* serviceInfo, service::reply::State& reply) override
            {
               // TODO: try to propagate the exception in the ballast, later on...

               common::error::handler();

               reply.code = TPFAIL;

               //auto type = buffer::type( serviceInfo->data);


               //reply.data = tpalloc( )

               reply.data = serviceInfo->data;

            }

         };


         std::unique_ptr< Interface> create( int argc, char **argv)
         {
            return std::unique_ptr< Interface>( new Implementation{});
         }

      } // server
   } // sf
} // casual



