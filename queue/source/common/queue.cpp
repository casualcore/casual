//!
//! queue.cpp
//!
//! Created on: Nov 30, 2014
//!     Author: Lazan
//!

#include "queue/common/queue.h"
#include "queue/common/environment.h"


#include "common/queue.h"


namespace casual
{
   namespace queue
   {

      Lookup::Lookup( const std::string& queue)
      {
         casual::common::queue::blocking::Writer send( queue::environment::broker::queue::id());

         common::message::queue::lookup::Request request;
         request.process = common::process::handle();
         request.name = queue;

         send( request);
      }

      common::message::queue::lookup::Reply Lookup::operator () () const
      {
         common::queue::blocking::Reader receive( common::ipc::receive::queue());

         common::message::queue::lookup::Reply reply;
         receive( reply);

         return reply;
      }


   } // queue
} // casual
