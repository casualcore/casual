//!
//! receiver.cpp
//!
//! Created on: May 6, 2015
//!     Author: Lazan
//!

#include "traffic/receiver.h"


#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/server/handle.h"
#include "common/communication/ipc.h"

#include "common/internal/trace.h"

namespace casual
{
   using namespace common;

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
               common::platform::pid::type get_pid() const { return message.process.pid;};
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
      common::platform::pid::type Event::pid() const { return get_pid(); }
      const common::Uuid& Event::execution() const { return get_execution(); }
      const common::transaction::ID& Event::transaction() const { return get_transaction(); }
      const common::platform::time_point& Event::start() const { return get_start(); }
      const common::platform::time_point& Event::end() const { return get_end(); }



      Receiver::Receiver( const common::Uuid& application)
      {
         common::trace::internal::Scope trace( "traffic::Receiver::Receiver( application)");

         //
         // Connect to domain
         //
         process::instance::connect( application);

         //
         // Register this traffic-logger with the broker
         //
         {
            message::traffic::monitor::connect::Request request;
            request.process = common::process::handle();

            communication::ipc::call( communication::ipc::broker::device(), request);

         }
      }

      Receiver::Receiver() : Receiver{ uuid::empty()}
      {
         common::trace::internal::Scope trace( "traffic::Receiver::Receiver");


      }

      Receiver::~Receiver()
      {
         common::trace::internal::Scope trace( "traffic::Receiver::~Receiver");

         //
         // We could try to process pending traffic messages, but if this
         // is a shutdown of this instance of traffic-logger and the rest of the system
         // is running at peek, we may never consume all messages, hence never shutdown.
         //

         //
         // TODO: do we need disconnect? Broker will eventually know that this process
         // has terminated.
         //
         message::traffic::monitor::Disconnect disconnect;
         disconnect.process = common::process::handle();
         communication::ipc::blocking::send( communication::ipc::broker::device(), disconnect);
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

            communication::ipc::Helper receiver;

            while( true)
            {

               log.persist_begin();

               //
               // Make sure we write persistent no matter what...
               //
               auto persist = common::scope::execute( [&](){
                  log.persist_commit();
               });


               //
               // Blocking
               //
               handler( receiver.blocking_next());


               //
               // Consume until the queue is empty or we've got pending events equal to statistics_batch
               //
               {
                  for( auto count = common::platform::batch::statistics;
                     handler( receiver.non_blocking_next()) && count > 0; --count)
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
