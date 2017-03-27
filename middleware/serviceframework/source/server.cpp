//!
//! casual
//!

#include "sf/server.h"


#include "common/log.h"

namespace casual
{
   namespace sf
   {
      class Server::Implementation
      {
      public:
         virtual ~Implementation() = default;
         virtual service::IO service( TPSVCINFO& information) = 0;
         virtual void exception( TPSVCINFO& information, service::reply::State& reply) = 0;
      };

      namespace server
      {
         class Implementation : public Server::Implementation
         {
         private:
            service::IO service( TPSVCINFO& information) override
            {
               return { sf::service::Factory::instance().create( &information)};
            }

            void exception( TPSVCINFO& information, service::reply::State& reply) override
            {
               // TODO: try to propagate the exception in the ballast, later on...

               common::error::handler();

               reply.code = TPFAIL;

               //auto type = buffer::type( serviceInfo->data);


               //reply.data = tpalloc( )

               reply.data = information.data;
            }
         };
      } // server

      Server::Server() : Server( 0, nullptr) {}

      Server::Server( int argc, char **argv) : m_implementation{ std::make_unique< server::Implementation>()}
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



