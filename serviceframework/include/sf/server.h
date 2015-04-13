//!
//! server.h
//!
//! Created on: Dec 27, 2012
//!     Author: Lazan
//!

#ifndef SERVER_H_
#define SERVER_H_

#include "xatmi.h"
#include "sf/service/interface.h"

#include <memory>

namespace casual
{
   namespace sf
   {
      namespace server
      {
         class Interface
         {
         public:
            service::IO createService( TPSVCINFO* serviceInfo);

            virtual ~Interface();

            void handleException( TPSVCINFO* serviceInfo, service::reply::State& reply);

         private:

            virtual std::unique_ptr< service::Interface> doCreateService( TPSVCINFO* serviceInfo) = 0;

            virtual void doHandleException( TPSVCINFO* serviceInfo, service::reply::State& reply) = 0;
         };

         using type = std::unique_ptr< Interface>;

         std::unique_ptr< Interface> create( int argc, char **argv);

         namespace implementation
         {
            template< typename T>
            using type = std::unique_ptr< T>;

            template< typename T>
            type< T> make( int argc, char **argv)
            {
               return type< T>{ new T{ argc, argv}};
            }
         }

         template< typename T>
         void sink( T&& server)
         {
            server.reset();
         }


      } // server
   } // sf
} // casual



#endif /* SERVER_H_ */
