//!
//! queue.cpp
//!
//! Created on: Jul 22, 2014
//!     Author: Lazan
//!


#include "queue/api/rm/queue.h"
#include "queue/common/queue.h"
#include "queue/common/environment.h"
#include "queue/common/transform.h"
#include "queue/rm/switch.h"


#include "common/message/queue.h"
#include "common/queue.h"
#include "common/trace.h"


#include "tx.h"



namespace casual
{
   namespace queue
   {

      namespace xatmi
      {
         /*
         struct Message::Implementation
         {

            common::Uuid id;
            std::string correlation;
            std::string reply;
            common::platform::time_type available;

            common::platform::raw_buffer_type payload;

         };

         Message::Message() = default;
         Message::~Message() = default;

         const common::Uuid& Message::id() const
         {
            return m_implementation->id;
         }

         const std::string& Message::correlation() const
         {
            return m_implementation->correlation;
         }

         Message& Message::correlation( std::string value)
         {
            m_implementation->correlation = std::move( value);
            return *this;
         }

         const std::string& Message::reply() const
         {
            return m_implementation->reply;
         }

         Message& Message::reply( std::string value)
         {
            m_implementation->reply = std::move( value);
            return *this;
         }

         common::platform::time_type Message::available() const
         {
            return m_implementation->available;
         }

         Message& Message::available( common::platform::time_type value)
         {
            m_implementation->available = std::move( value);
            return *this;
         }

         common::platform::raw_buffer_type Message::payload() const
         {
            return m_implementation->payload;
         }

         Message& Message::payload( common::platform::raw_buffer_type value)
         {
            m_implementation->payload = value;
            return *this;
         }
         */

      } // xatmi

      namespace rm
      {
         namespace local
         {
            namespace
            {

               namespace scoped
               {
                  struct AX_reg
                  {
                     AX_reg( common::transaction::ID& trid) : m_id( trid)
                     {
                        ax_reg( queue::rm::id(), &m_id.xid, TMNOFLAGS);
                     }

                     ~AX_reg()
                     {
                        if( ! m_id)
                        {
                           ax_unreg( queue::rm::id(), TMNOFLAGS);
                        }

                     }
                  private:
                     common::transaction::ID& m_id;
                  };
               } // scoped


            } // <unnamed>
         } // local


         sf::platform::Uuid enqueue( const std::string& queue, const Message& message)
         {
            common::trace::Scope trace( "queue::rm::enqueue", common::log::internal::queue);

            //
            // Send the request
            //
            {
               queue::Lookup lookup( queue);

               common::message::queue::enqueue::Request request;
               local::scoped::AX_reg ax_reg( request.trid);

               request.process = common::process::handle();

               request.message.payload = message.payload.data;
               request.message.type.type = message.payload.type.type;
               request.message.type.subtype = message.payload.type.subtype;
               request.message.correlation = message.attribues.properties;
               request.message.reply = message.attribues.reply;
               request.message.avalible = message.attribues.available;
               //request.message.id = common::Uuid::make();

               auto group = lookup();

               if( group.queue == 0)
               {
                  throw common::exception::invalid::Argument{ "failed to look up queue: " + queue};
               }

               common::log::internal::queue << "enqueues - queue: " << queue << " group: " << group.queue << " process: " << group.process << std::endl;


               casual::common::queue::blocking::Writer send( group.process.queue);
               request.queue = group.queue;

               send( request);
            }

            common::message::queue::enqueue::Reply reply;

            //
            // Get the reply
            //
            {
               casual::common::queue::blocking::Reader receive{ common::ipc::receive::queue()};
               receive( reply);
            }


            return reply.id;
         }


         std::vector< Message> dequeue( const std::string& queue)
         {
            common::trace::Scope trace( "queue::rm::dequeue", common::log::internal::queue);

            std::vector< Message> result;

            queue::Lookup lookup( queue);

            common::message::queue::dequeue::Request request;
            local::scoped::AX_reg ax_reg( request.trid);

            {

               request.process = common::process::handle();

               auto group = lookup();
               casual::common::queue::blocking::Writer send( group.process.queue);
               request.queue = group.queue;

               common::log::internal::queue << "dequeues - queue: " << queue << " group: " << group.queue << " process: " << group.process << std::endl;

               send( request);
            }

            {
               casual::common::queue::blocking::Reader receive( common::ipc::receive::queue());
               common::message::queue::dequeue::Reply reply;

               receive( reply);

               common::range::transform( reply.message, result, queue::transform::Message());
            }

            return result;
         }


         namespace peek
         {
            namespace local
            {
               namespace
               {
                  namespace transform
                  {
                     struct Message
                     {
                        queue::peek::Message operator () ( const common::message::queue::information::Message& message) const
                        {
                           queue::peek::Message result;

                           result.id = message.id;
                           //result.type = message.type;
                           result.state = message.state;

                           return result;
                        }
                     };
                  } // transform

               } // <unnamed>
            } // local

            std::vector< queue::peek::Message> queue( const std::string& queue)
            {
               std::vector< queue::peek::Message> result;

               {
                  casual::common::queue::blocking::Writer send( queue::environment::broker::queue::id());

                  common::message::queue::information::queue::Request request;
                  request.process = common::process::handle();
                  request.qname = queue;

                  send( request);
               }

               {

                  common::queue::blocking::Reader receive( common::ipc::receive::queue());

                  common::message::queue::information::queue::Reply reply;
                  receive( reply);

                  common::range::transform( reply.messages, result, local::transform::Message{});

               }
               return result;
            }

         } // peek
      } // rm
   } // queue
} // casual
