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
            std::unique_ptr< service::Interface> service( );

            virtual ~Interface();

         private:

            virtual std::unique_ptr< service::Interface> doService() = 0;
         };

         std::unique_ptr< Interface> create();


      } // server
   } // sf
} // casual



#endif /* SERVER_H_ */
