//!
//! receiver.h
//!
//! Created on: May 6, 2015
//!     Author: Lazan
//!

#ifndef TRAFFIC_RECEIVER_H_
#define TRAFFIC_RECEIVER_H_


// TODO: change name to traffic.h
#include "common/message/monitor.h"




namespace casual
{
   namespace traffic
   {
      //
      // hanler::Base and Receiver could be in one header and source-file
      //

      using message_type = common::message::traffic::monitor::Notify;

      namespace handler
      {
         struct Base
         {
            Base() = default;
            virtual ~Base() = default;

            virtual void persist_begin() = 0;
            virtual void log( const message_type&) = 0;
            virtual void persist_commit() = 0;
         };

      } // handler


      struct Receiver
      {
      public:

         Receiver();
         ~Receiver();

         Receiver( const Receiver&) = delete;
         Receiver& operator = ( const Receiver&) = delete;


         int start( handler::Base& log);
      };

   } // traffic
} // casual


#endif // RECEIVER_H_
