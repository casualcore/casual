//!
//! casual
//!

#ifndef SERVER_H_
#define SERVER_H_


#include "sf/service/interface.h"

#include "common/pimpl.h"

#include "xatmi/defines.h"


#include <memory>

namespace casual
{
   namespace sf
   {

      class Server
      {
      public:

         Server();
         Server( int argc, char **argv);
         ~Server();


         service::IO service( TPSVCINFO& information);
         void exception( TPSVCINFO& information, service::reply::State& reply);

      private:
         struct Implementation;
         common::basic_pimpl< Implementation> m_implementation;

      };

      namespace server
      {
         namespace implementation
         {
            template< typename T>
            using type =  std::unique_ptr< T>;

            template< typename T>
            auto make( int argc, char **argv)
            {
               return std::make_unique< T>( argc, argv);
            }

            template< typename T>
            void sink( type< T>&& value)
            {
            }

         } // implementation
         void sink( sf::Server&& server);
      } // server
   } // sf
} // casual



#endif /* SERVER_H_ */
