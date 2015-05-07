/*
 * casual_monitor.h
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#ifndef CASUAL_RECEIVER_H_
#define CASUAL_RECEIVER_H_

#include <vector>
#include <string>

#include "common/ipc.h"
#include "common/message/monitor.h"

#include "common/queue.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/server/handle.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/error.h"
#include "common/log.h"
#include "common/trace.h"
#include "common/environment.h"
#include "common/chronology.h"

using namespace casual::common;

namespace casual
{
namespace traffic
{
   template < class R, class H>
   class Receiver
   {
   public:

      typedef R resource_type;
      typedef H handler_type;

      Receiver( const std::vector< std::string>& arguments, resource_type& resource) :
         m_receiveQueue( common::ipc::receive::queue()),
         m_resource( resource)
      {
         //
         // TODO: Use a correct argumentlist handler
         //
         const std::string name = ! arguments.empty() ? arguments.front() : std::string("");

         common::process::path( name);

         trace::internal::Scope trace( "Receiver::Receiver");

         //
         // Connect as a "regular" server
         //
         common::server::connect( {});


         //
         // Make the key public for others...
         //
         message::traffic::monitor::Connect message;

         message.path = name;
         message.process = common::process::handle();

         queue::blocking::Writer writer( ipc::broker::id());
         writer(message);

      }
      ~Receiver()
      {
         trace::internal::Scope trace( "Receiver::~Receiver");

         try
         {

            message::dispatch::Handler handler{
               handler_type{ m_resource},
               common::message::handle::Shutdown{},
            };

            typename resource_type::transaction_type transaction{ m_resource};

            //
            // Consume until the queue is empty or we've got pending replies equal to statistics_batch
            //

            queue::non_blocking::Reader nonBlocking( m_receiveQueue);

            for( auto count = common::platform::batch::statistics;
               handler( nonBlocking.next()) && count > 0; --count)
            {
               ;
            }
         }
         catch( ...)
         {
            common::error::handler();
            return;
         }

      }


      //!
      //! Start the server
      //!
      void start()
      {
         trace::internal::Scope trace( "Receiver::start");

         message::dispatch::Handler handler{
            handler_type{ m_resource},
            common::message::handle::Shutdown{},
         };

         queue::blocking::Reader queueReader(m_receiveQueue);

         while( true)
         {

            typename resource_type::transaction_type transaction{ m_resource};

            //
            // Blocking
            //

            handler( queueReader.next());


            //
            // Consume until the queue is empty or we've got pending replies equal to statistics_batch
            //

            queue::non_blocking::Reader nonBlocking( m_receiveQueue);

            for( auto count = common::platform::batch::statistics;
               handler( nonBlocking.next()) && count > 0; --count)
            {
               ;
            }
         }

      }

   private:
      common::ipc::receive::Queue& m_receiveQueue;
      resource_type& m_resource;
   };
}
}


#endif /* CASUAL_RECEIVER_H_ */
