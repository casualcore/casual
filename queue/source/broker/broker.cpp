//!
//! broker.cpp
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#include "queue/broker/broker.h"


#include "queue/broker/handle.h"


#include "common/message/dispatch.h"
#include "common/ipc.h"

namespace casual
{

   namespace queue
   {
      namespace broker
      {



      } // broker


      Broker::Broker( broker::Settings settings)
      {

      }

      void Broker::start()
      {

         common::message::dispatch::Handler handler;

         handler.add( broker::handle::lookup::Request{ m_state});

         broker::queue::blocking::Reader blockedRead( common::ipc::receive::queue(), m_state);

         while( true)
         {
            handler.dispatch( blockedRead.next());
         }

      }

   } // queue



} // casual
