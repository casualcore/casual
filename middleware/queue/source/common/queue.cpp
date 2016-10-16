//!
//! casual
//!

#include "queue/common/queue.h"
#include "queue/common/environment.h"


#include "common/communication/ipc.h"


namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            common::message::queue::lookup::Request request( const std::string& queue)
            {
               common::message::queue::lookup::Request request;
               request.process = common::process::handle();
               request.name = queue;

               return request;
            }
         } // <unnamed>
      } // local

      Lookup::Lookup( const std::string& queue)
         : m_correlation{
            common::communication::ipc::blocking::send(
                  common::communication::ipc::queue::broker::optional::device(),
                  local::request( queue))}
      {
      }

      common::message::queue::lookup::Reply Lookup::operator () () const
      {
         common::message::queue::lookup::Reply reply;

         common::communication::ipc::blocking::receive(
               common::communication::ipc::inbound::device(),
               reply,
               m_correlation);

         return reply;
      }


   } // queue
} // casual
