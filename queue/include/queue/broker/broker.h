//!
//! broker.h
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#ifndef QUEUE_BROKER_BROKER_H_
#define QUEUE_BROKER_BROKER_H_



#include <string>
#include <unordered_map>

namespace casual
{
   namespace queue
   {
      namespace broker
      {
         struct Settings
         {
            std::string configuration;
         };

         struct State
         {
            struct Server
            {

            };


            std::unordered_map< std::string, Server> queues;
         };
      } // broker

      struct Broker
      {
         Broker( broker::Settings settings);

         void start();

      private:
         broker::State m_state;

      };

   } // queue
} // casual

#endif // BROKER_H_
