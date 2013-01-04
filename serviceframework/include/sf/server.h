//!
//! server.h
//!
//! Created on: Dec 27, 2012
//!     Author: Lazan
//!

#ifndef SERVER_H_
#define SERVER_H_

#include "xatmi.h"
#include "sf/service.h"

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
            std::unique_ptr< service::Interface> service( TPSVCINFO* serviceInfo);

            virtual ~Interface();

         private:

            virtual std::unique_ptr< service::Interface> doGetService( TPSVCINFO* serviceInfo) = 0;
         };

         std::unique_ptr< Interface> create( int argc, char **argv);


      } // server
   } // sf
} // casual



#endif /* SERVER_H_ */
