//!
//! receiver.cpp
//!
//! Created on: May 6, 2015
//!     Author: Lazan
//!

#include "proposal/receiver.h"


#include "common/queue.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/server/handle.h"

#include "common/internal/trace.h"

namespace casual
{
   namespace traffic
   {
      namespace local
      {
         namespace
         {
            struct handle_traffic
            {
               using message_type = traffic::message_type;

               handle_traffic( handler::Base& handler) : m_handler( handler) {}

               void operator () ( message_type& message)
               {
                  m_handler.log( message);
               }

            private:
               handler::Base& m_handler;

            };
         } // <unnamed>
      } // local
      Receiver::Receiver()
      {
         common::trace::internal::Scope trace( "traffic::Receiver::Receiver");

         //
         // Connect as a "regular" server
         //
         common::server::connect( {});

         //
         // Register this traffic-logger
         //
         {
            common::message::traffic::monitor::Connect message;

            message.path = common::process::path();
            message.process = common::process::handle();

            common::queue::blocking::Send send;
            send( common::ipc::broker::id(), message);
         }
      }

      Receiver::~Receiver()
      {
         //
         // We could try to process pending traffic messages, but if this
         // is a shutdown of this instance of traffic-logger and the rest of the system
         // is running at peek, we may never consume all messages, hence never shutdown.
         //
      }


      int Receiver::start( handler::Base& log)
      {
         try
         {
            common::message::dispatch::Handler handler{
               local::handle_traffic{ log},
               common::message::handle::Shutdown{},
               common::message::handle::ping(),
            };

            common::queue::blocking::Reader reader{ common::ipc::receive::queue()};

            while( true)
            {

               //
               // Make sure we write persistent no matter what...
               //
               common::scope::Execute persist{ [&](){
                  log.persist();
               }};


               //
               // Blocking
               //
               handler( reader.next());


               //
               // Consume until the queue is empty or we've got pending replies equal to statistics_batch
               //
               {
                  common::queue::non_blocking::Reader non_blocking( common::ipc::receive::queue());

                  for( auto count = common::platform::batch::statistics;
                     handler( non_blocking.next()) && count > 0; --count)
                  {
                     ;
                  }
               }
            }
         }
         catch( ...)
         {
            return common::error::handler();
         }
         return 0;
      }

   } // traffic
} // casual
