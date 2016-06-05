//!
//! casual
//!

#include "queue/common/queue.h"
#include "queue/common/environment.h"


#include "common/communication/ipc.h"


namespace casual
{
   using namespace common;
   namespace queue
   {

      Lookup::Lookup( const std::string& queue)
      {
         common::message::queue::lookup::Request request;
         request.process = common::process::handle();
         request.name = queue;

         common::communication::ipc::blocking::send( communication::ipc::queue::broker::device(), request);
      }

      common::message::queue::lookup::Reply Lookup::operator () () const
      {
         common::message::queue::lookup::Reply reply;

         common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);

         return reply;
      }


   } // queue
} // casual
