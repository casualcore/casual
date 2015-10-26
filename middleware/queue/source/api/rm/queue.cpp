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
#include "common/message/handle.h"
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
                     AX_reg()
                     {
                        ax_reg( queue::rm::id(), &trid.xid, TMNOFLAGS);
                     }

                     ~AX_reg()
                     {
                        if( ! trid)
                        {
                           ax_unreg( queue::rm::id(), TMNOFLAGS);
                        }
                     }

                     common::transaction::ID trid;
                  private:

                  };
               } // scoped


               sf::platform::Uuid enqueue( const std::string& queue, const Message& message)
               {
                  //
                  // Register to TM
                  //
                  local::scoped::AX_reg ax_reg;

                  auto send_request = [&]()
                  {
                     queue::Lookup lookup( queue);

                     common::message::queue::enqueue::Request request;
                     request.trid = ax_reg.trid;

                     request.process = common::process::handle();

                     request.message.payload = message.payload.data;
                     request.message.type.name = message.payload.type.type;
                     request.message.type.subname = message.payload.type.subtype;
                     request.message.properties = message.attributes.properties;
                     request.message.reply = message.attributes.reply;
                     request.message.avalible = message.attributes.available;

                     auto group = lookup();

                     if( group.queue == 0)
                     {
                        throw common::exception::invalid::Argument{ "failed to look up queue: " + queue};
                     }
                     request.queue = group.queue;

                     common::log::internal::queue << "enqueues - queue: " << queue << " group: " << group.queue << " process: " << group.process << std::endl;
                     common::log::internal::queue << "enqueues - request: " << request << std::endl;

                     casual::common::queue::blocking::Send send;
                     return send( group.process.queue, request);
                  };

                  common::message::queue::enqueue::Reply reply;

                  casual::common::queue::blocking::Reader receive{ common::ipc::receive::queue()};
                  receive( reply, send_request());

                  return reply.id;
               }


               std::vector< Message> dequeue( const std::string& queue, const Selector& selector, bool block = false)
               {

                  //
                  // Register to TM
                  //
                  local::scoped::AX_reg ax_reg;

                  common::scope::Execute forget_blocking{ [&]()
                  {
                     queue::Lookup lookup( queue);

                     common::message::queue::dequeue::forget::Request request;
                     request.process = common::process::handle();

                     auto group = lookup();
                     request.queue = group.queue;

                     casual::common::queue::blocking::Send send;
                     auto correlation = send( group.process.queue, request);

                     casual::common::queue::blocking::Reader receive{ common::ipc::receive::queue()};
                     common::message::queue::dequeue::forget::Reply reply;
                     receive( reply, correlation);
                  }};

                  auto send_reqeust = [&]()
                  {
                     queue::Lookup lookup( queue);

                     common::message::queue::dequeue::Request request;
                     request.trid = ax_reg.trid;

                     request.process = common::process::handle();

                     auto group = lookup();
                     request.queue = group.queue;
                     request.block = block;
                     request.selector.id = selector.id;
                     request.selector.properties = selector.properties;

                     common::log::internal::queue << "async::dequeue - request: " << request << std::endl;

                     casual::common::queue::blocking::Send send;
                     return send( group.process.queue, request);
                  };

                  auto correlation = send_reqeust();

                  std::vector< Message> result;

                  casual::common::queue::blocking::Reader receive{ common::ipc::receive::queue()};

                  //
                  // We need to listen to shutdown-message.
                  // TODO: Don't know if we really should do this here, but otherwise we have
                  // no way of "interrupt" if it's a blocking request. We could rely only on terminate-signal
                  // (which we now also do) but it isn't really coherent with how casual otherwise works
                  //
                  auto complete = receive.next( {
                     common::message::queue::dequeue::Reply::message_type,
                     common::message::shutdown::Request::message_type});

                  if( complete.type == common::message::queue::dequeue::Reply::message_type)
                  {
                     common::message::queue::dequeue::Reply reply;
                     complete >> reply;

                     if( reply.correlation != correlation)
                     {
                        throw common::exception::NotReallySureWhatToNameThisException{ "correlation mismatch"};
                     }

                     common::range::transform( reply.message, result, queue::transform::Message());
                  }
                  else
                  {
                     common::log::internal::queue << "async::dequeue::reply - shutdown received" << std::endl;

                     common::message::shutdown::Request request;
                     complete >> request;

                     common::message::handle::Shutdown{}( request);
                  }

                  //
                  // We don't need to send forget, since it went as it should.
                  //
                  forget_blocking.release();

                  return result;
               }

            } // <unnamed>
         } // local


         sf::platform::Uuid enqueue( const std::string& queue, const Message& message)
         {
            common::trace::Scope trace( "queue::rm::enqueue", common::log::internal::queue);

            return local::enqueue( queue, message);
         }

         std::vector< Message> dequeue( const std::string& queue, const Selector& selector)
         {
            common::trace::Scope trace( "queue::rm::dequeue", common::log::internal::queue);

            return local::dequeue( queue, selector);
         }

         std::vector< Message> dequeue( const std::string& queue)
         {
            return dequeue( queue, Selector{});
         }



         namespace blocking
         {
            Message dequeue( const std::string& queue, const Selector& selector)
            {
               common::trace::Scope trace( "queue::rm::blocking::dequeue", common::log::internal::queue);

               auto message = local::dequeue( queue, selector, true);

               if( message.empty())
               {
                  throw common::exception::NotReallySureWhatToNameThisException{ "blocking dequeue replied empty message"};
               }

               return std::move( message.front());
            }

            Message dequeue( const std::string& queue)
            {
               return dequeue( queue, Selector{});
            }


         } // blocking


         /*
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
         */
      } // rm
   } // queue
} // casual
