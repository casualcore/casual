//!
//! casual
//!

#include "sf/server.h"


#include "common/log.h"

namespace casual
{
   namespace sf
   {


      struct Server::Implementation
      {
         service::IO service( TPSVCINFO& information)
         {
            return { sf::service::factory().create( &information)};
         }

         void exception( TPSVCINFO& information, service::reply::State& reply)
         {
            // TODO: try to propagate the exception in the ballast, later on...

            common::error::handler();

            reply.code = TPFAIL;

            //auto type = buffer::type( serviceInfo->data);


            //reply.data = tpalloc( )

            reply.data = information.data;
         }
      };


      Server::Server() : Server( 0, nullptr) {}

      Server::Server( int argc, char **argv)
      {

      }
      Server::~Server() = default;

      service::IO Server::service( TPSVCINFO& information)
      {
         return m_implementation->service( information);
      }

      void Server::exception( TPSVCINFO& information, service::reply::State& reply)
      {
         m_implementation->exception( information, reply);
      }

      namespace server
      {
         void sink( sf::Server&& server)
         {

         }
      } // server

   } // sf
} // casual



