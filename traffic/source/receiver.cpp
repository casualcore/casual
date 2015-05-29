//!
//! receiver.cpp
//!
//! Created on: May 6, 2015
//!     Author: Lazan
//!

#include "traffic/receiver.h"


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

            struct Event : traffic::Event
            {
               using message_type = common::message::traffic::Event;

               Event( message_type& message) : message( message) {}

               const std::string& get_service() const { return message.service;};
               const std::string& get_parent() const { return message.parent;};
               common::platform::pid_type get_pid() const { return message.process.pid;};
               const common::Uuid& get_execution() const { return message.execution;};
               const common::transaction::ID& get_transaction() const { return message.trid;};
               const common::platform::time_point& get_start() const { return message.start;};
               const common::platform::time_point& get_end() const { return message.end;};

               message_type& message;
            };


            struct handle_traffic
            {
               using message_type = Event::message_type;

               handle_traffic( handler::Base& handler) : m_handler( handler) {}

               void operator () ( message_type& message)
               {
                  Event event{ message};
                  m_handler.log( event);
               }

            private:
               handler::Base& m_handler;

            };



         } // <unnamed>
      } // local


      Event::Event() = default;

      const std::string& Event::service() const { return get_service(); }
      const std::string& Event::parent() const { return get_parent(); }
      common::platform::pid_type Event::pid() const { return get_pid(); }
      const common::Uuid& Event::execution() const { return get_execution(); }
      const common::transaction::ID& Event::transaction() const { return get_transaction(); }
      const common::platform::time_point& Event::start() const { return get_start(); }
      const common::platform::time_point& Event::end() const { return get_end(); }



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

               log.persist_begin();

               //
               // Make sure we write persistent no matter what...
               //
               common::scope::Execute persist{ [&](){
                  log.persist_commit();
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
