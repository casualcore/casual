//!
//! queue.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/server/server.h"


namespace casual
{
   namespace queue
   {

      namespace server
      {

         Server::Server( Settings settings) : m_state( std::move( settings))
         {
         }


         void Server::start()
         {

         }
      } // server

   } // queue

} // casual
