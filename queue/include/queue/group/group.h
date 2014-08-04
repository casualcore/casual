//!
//! queue.h
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#ifndef CASUALQUEUESERVER_H_
#define CASUALQUEUESERVER_H_

#include <string>

#include "queue/group/database.h"

#include "common/platform.h"


namespace casual
{
   namespace queue
   {
      namespace group
      {
         struct Settings
         {
            std::string queuebase;
         };

         struct State
         {
            State( std::string filename) : queuebase( std::move( filename)) {}

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
