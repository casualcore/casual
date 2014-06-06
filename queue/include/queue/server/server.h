//!
//! queue.h
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#ifndef CASUALQUEUESERVER_H_
#define CASUALQUEUESERVER_H_

#include <string>


namespace casual
{
   namespace queue
   {

      struct Settings
      {
         std::string queuebase;
      };


      struct Server
      {
         Server( Settings settings);


         void start();

      };

   } // queue

} // casual

#endif // QUEUE_H_
