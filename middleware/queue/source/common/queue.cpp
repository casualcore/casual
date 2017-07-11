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

      Lookup::Lookup( std::string queue)
         : m_name( std::move( queue)), m_correlation{
            common::communication::ipc::blocking::send(
                  common::communication::ipc::queue::manager::optional::device(),
                  local::request( m_name))}
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

      const std::string& Lookup::name() const
      {
         return m_name;
      }


   } // queue
} // casual
