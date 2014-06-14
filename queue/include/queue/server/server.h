//!
//! queue.h
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#ifndef CASUALQUEUESERVER_H_
#define CASUALQUEUESERVER_H_

#include <string>

#include "queue/server/database.h"


namespace casual
{
   namespace queue
   {
      namespace server
      {
         struct Settings
         {
            std::string queuebase;
         };

         struct State
         {
            State( Settings settings) : queuebase( std::move( settings.queuebase)) {}

            Database queuebase;
         };


         struct Server
         {
            Server( Settings settings);


            void start();

         private:
            State m_state;
         };
      } // server

   } // queue

} // casual

#endif // QUEUE_H_
