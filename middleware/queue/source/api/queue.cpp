//!
//! casual
//!

#include "queue/api/queue.h"
#include "queue/common/log.h"
#include "queue/common/queue.h"
#include "queue/common/transform.h"
#include "queue/manager/admin/services.h"

#include "common/buffer/type.h"
#include "common/buffer/pool.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/transaction/context.h"
#include "common/communication/ipc.h"
#include "common/execute.h"


#include "sf/service/protocol/call.h"
#include "sf/log.h"

namespace casual
{
   namespace queue
   {
      inline namespace v1
      {
         namespace local
         {
            namespace
            {

               namespace exception
               {
                  struct Lookup : common::exception::system::invalid::Argument
                  {
                     using common::exception::system::invalid::Argument::Argument;
                  };
               } // exception

               template< typename M>
               sf::platform::Uuid enqueue( const queue::Lookup& lookup, M&& message)
               {
                  Trace trace( "casual::queue::enqueue");

                  common::log::line( verbose::log, "message: ", message);

                  auto& transaction = common::transaction::context().current();

                  if( transaction)
                  {
                     //
                     // Make sure we trigger an interaction with the TM.
                     // Since the queue-groups act as 'external resources' to
                     // the TM
                     //
                     transaction.external();
                  }


                  common::message::queue::enqueue::Request request;
                  request.trid = transaction.trid;

                  request.process = common::process::handle();

                  request.message.payload = message.payload.data;
                  request.message.type = message.payload.type;
                  request.message.properties = message.attributes.properties;
                  request.message.reply = message.attributes.reply;
                  request.message.available = message.attributes.available;

                  request.name = lookup.name();

                  auto group = lookup();

                  if( ! group.process.queue)
                  {
                     throw common::exception::system::invalid::Argument{ "failed to look up queue"};
                  }
                  request.queue = group.queue;

                  common::log::line( verbose::log, "request: ", request);

                  return common::communication::ipc::call( group.process.queue, request).id;

               }


               std::vector< Message> dequeue( const queue::Lookup& lookup, const Selector& selector, bool block = false)
               {
                  Trace trace{ "casual::queue::local::dequeue"};


                  auto& transaction = common::transaction::context().current();


                  casual::common::communication::ipc::Helper ipc;

                  auto group = lookup();

                  if( ! group)
                  {
                     throw exception::Lookup{ "failed to lookup queue: " + lookup.name()};
                  }


                  auto forget_blocking = common::execute::scope( [&]()
                  {
                     common::message::queue::dequeue::forget::Request request;
                     request.process = common::process::handle();

                     request.queue = group.queue;

                     try
                     {
                        ipc.blocking_send( group.process.queue, request);

                        auto handler = ipc.handler(
                           []( common::message::queue::dequeue::forget::Request& request)
                           {
                              // no-op
                           },
                           []( common::message::queue::dequeue::forget::Reply& request)
                           {
                              // no-op
                           }
                        );

                        handler( ipc.blocking_next( handler.types()));
                     }
                     catch( const common::exception::system::communication::Unavailable&)
                     {
                        // queue-manager is off-line
                     }
                  });

                  auto send_request = [&]()
                  {
                     common::message::queue::dequeue::Request request;
                     request.trid = transaction.trid;

                     request.process = common::process::handle();

                     request.queue = group.queue;
                     request.block = block;
                     request.name = lookup.name();
                     request.selector.id = selector.id;
                     request.selector.properties = selector.properties;

                     common::log::line( verbose::log, "request: ", request);

                     return ipc.blocking_send( group.process.queue, request);
                  };

                  auto correlation = send_request();

                  std::vector< Message> result;


                  //
                  // We need to listen to shutdown-message.
                  // TODO: Don't know if we really should do this here, but otherwise we have
                  // no way of "interrupt" if it's a blocking request. We could rely only on terminate-signal
                  // (which we now also do) but it isn't really coherent with how casual otherwise works
                  //

                  auto handler = ipc.handler(
                     [&]( common::message::queue::dequeue::Reply& reply)
                     {
                        if( ! reply.message.empty() && transaction)
                        {
                           //
                           // Make sure we trigger an interaction with the TM.
                           // Since the queue-groups act as 'external resources' to
                           // the TM
                           //
                           transaction.external();
                        }

                        if( reply.correlation != correlation)
                        {
                           throw common::exception::system::invalid::Argument{ "correlation mismatch"};
                        }
                        common::algorithm::transform( reply.message, result, queue::transform::Message{});

                     },
                     [&]( common::message::queue::dequeue::forget::Request& request)
                     {
                        //
                        // The group we're waiting for is going off-line, we just
                        // return an empty message.
                        //
                        // We don't need to send forget to group ( since that is exactly what
                        // it is telling us...)
                        //
                        forget_blocking.release();
                     },
                     common::message::handle::Shutdown{},
                     common::message::handle::ping()
                  );

                  handler( ipc.blocking_next());



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
            Trace trace( "casual::queue::enqueue");

            queue::Lookup lookup( queue);

            return local::enqueue( lookup, message);
         }

         std::vector< Message> dequeue( const std::string& queue, const Selector& selector)
         {
            Trace trace{ "casual::queue::dequeue"};

            queue::Lookup lookup( queue);

            return local::dequeue( lookup, selector, false);
         }

         std::vector< Message> dequeue( const std::string& queue)
         {
            return dequeue( queue, Selector{});
         }

         namespace blocking
         {
            Message dequeue( const std::string& queue)
            {
               Trace trace{ "casual::queue::blocking::dequeue"};

               return blocking::dequeue( queue, Selector{});
            }

            Message dequeue( const std::string& queue, const Selector& selector)
            {
               Trace trace{ "casual::queue::blocking::dequeue"};

               queue::Lookup lookup( queue);

               auto message = local::dequeue( lookup, selector, true);

               if( message.empty())
                  throw common::exception::system::communication::no::message::Absent{ "failed to get message from queue: " + queue};

               return std::move( message.front());
            }

            namespace available
            {
               Message dequeue( const std::string& queue)
               {
                  return available::dequeue( queue, Selector{});
               }

               Message dequeue( const std::string& queue, const Selector& selector)
               {
                  Trace trace{ "casual::queue::blocking::available::dequeue"};

                  common::process::pattern::Sleep sleep{
                     { std::chrono::milliseconds{ 100}, 10},
                     { std::chrono::seconds{ 2}, 100},
                     { std::chrono::seconds{ 10}, common::process::pattern::Sleep::Pattern::infinite_quantity{}},
                  };

                  while( true)
                  {
                     try
                     {
                        return blocking::dequeue( queue, selector);
                     }
                     catch( const local::exception::Lookup&)
                     {
                        common::log::line( verbose::log, queue, " - queue not available yet"); 
                     }
                     sleep();
                  }
               }

            } // available

         } // blocking

         namespace xatmi
         {
            namespace copy
            {
               //
               // To hold reference data, so we don't need to copy the buffer.
               //
               struct Payload
               {
                  template< typename T, typename Iter>
                  Payload( T&& type, Iter first, Iter last)
                    : type( type), data( first, last)
                  {

                  }

                  Payload( Payload&&) = default;
                  Payload& operator = ( Payload&&) = default;

                  std::string type;
                  sf::platform::binary::type data;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( type);
                     archive & CASUAL_MAKE_NVP( data);
                  })
               };

               using Message = basic_message< Payload>;


            } // reference

            sf::platform::Uuid enqueue( const std::string& queue, const Message& message)
            {
               Trace trace{ "casual::queue::xatmi::enqueue"};

               const queue::Lookup lookup{ queue};

               auto send = common::buffer::pool::Holder::instance().get( message.payload.buffer, message.payload.size);

               //
               // We have to send only the real size of the buffer [buffer.begin, buffer.begin + transport_size)
               //
               // We could do this in several ways, but the most clean end ease way is to do
               // a copy... This is probably the only place we do a copy of a buffer.
               //
               // TODO: get rid of the copy in a conformant way
               //         we probably need to change the interface for 'binary' in write-archives (to take a range, or iterator first, last)
               //
               copy::Message send_message{
                  message.id, message.attributes,
                    { send.payload().type, send.payload().memory.begin(), send.payload().memory.begin() + send.transport}};

               return local::enqueue( lookup, send_message);
            }



            std::vector< Message> dequeue( const std::string& queue, const Selector& selector)
            {
               Trace trace{ "casual::queue::xatmi::dequeue"};

               std::vector< Message> results;

               for( auto& message : queue::dequeue( queue, selector))
               {
                  Message result;
                  result.attributes = std::move( message.attributes);
                  result.id = std::move( message.id);

                  {
                     common::buffer::Payload payload;
                     payload.type = std::move( message.payload.type);
                     payload.memory = std::move( message.payload.data);

                     std::tie( result.payload.buffer, result.payload.size) =
                           common::buffer::pool::Holder::instance().insert( std::move( payload));
                  }

                  results.push_back( std::move( result));
               }
               return results;
            }

            std::vector< Message> dequeue( const std::string& queue)
            {
               return xatmi::dequeue( queue, Selector{});
            }

         } // xatmi

         namespace peek
         {
            std::vector< message::Information> information( const std::string& queue)
            {
               return peek::information( queue, Selector{});
            }

            std::vector< message::Information> information( const std::string& queuename, const Selector& selector)
            {
               Trace trace{ "casual::queue::peek::information"};

               queue::Lookup lookup{ queuename};

               std::vector< message::Information> result;

               common::message::queue::peek::information::Request request;
               request.name = queuename;
               request.process = common::process::handle();
               request.selector.properties = selector.properties;



               auto queue = lookup();

               if( queue.order > 0)
               {
                  throw common::exception::system::invalid::Argument{ "not possible to peek a remote queue"};
               }

               request.queue = queue.queue;

               auto reply = common::communication::ipc::call( queue.process.queue, request);

               common::algorithm::transform( reply.messages, result, []( common::message::queue::information::Message& m){

                  message::Information message;
                  message.id = m.id;
                  message.trid = std::move( m.trid);
                  message.state = m.state;
                  message.attributes.available = m.available;
                  message.attributes.reply = std::move( m.reply);
                  message.attributes.properties = std::move( m.properties);
                  message.payload.type = std::move( m.type);
                  message.payload.size = m.size;
                  message.redelivered = m.redelivered;
                  message.timestamp = m.timestamp;

                  return message;
               });

               return result;
            }

            std::vector< Message> messages( const std::string& queuename, const std::vector< queue::Message::id_type>& ids)
            {
               Trace trace{ "casual::queue::peek::messages"};

               queue::Lookup lookup{ queuename};

               std::vector< Message> result;

               common::message::queue::peek::messages::Request request;
               {
                  request.process = common::process::handle();
                  request.ids = ids;
               }

               auto queue = lookup();

               if( queue.order > 0)
               {
                  throw common::exception::system::invalid::Argument{ "not possible to peek a remote queue"};
               }

               auto reply = common::communication::ipc::call( queue.process.queue, request);

               common::algorithm::transform( reply.messages , result, []( common::message::queue::dequeue::Reply::Message& m){
                  Message message;
                  message.id = m.id;
                  message.attributes.available = m.available;
                  message.attributes.reply = std::move( m.reply);
                  message.attributes.properties = std::move( m.properties);
                  message.payload.type = std::move( m.type);
                  message.payload.data = std::move( m.payload);
                  return message;
               });

               return result;
            }
         } // peek

         namespace restore
         {
            std::vector< Affected> queue( const std::vector< std::string>& queues)
            {
               Trace trace{ "casual::queue::restore::queue"};

               std::vector< Affected> affected;

               for( auto& queue : queues)
               {
                  sf::service::protocol::binary::Call call;
                  call << CASUAL_MAKE_NVP( queue);

                  auto reply = call( manager::admin::service::name::restore());

                  std::vector< manager::admin::Affected> result;
                  reply >> CASUAL_MAKE_NVP( result);

                  common::algorithm::transform( result, affected, []( const manager::admin::Affected& a){
                     Affected result;
                     result.queue = a.queue.name;
                     result.restored = a.restored;
                     return result;
                  });
               }

               return affected;
            }
         } // restore
      }

   } // queue
} // casual
