//!
//! casual
//!

#include "queue/api/queue.h"
#include "queue/common/log.h"
#include "queue/common/queue.h"
#include "queue/common/transform.h"

#include "common/buffer/type.h"
#include "common/buffer/pool.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/transaction/context.h"
#include "common/communication/ipc.h"


#include "sf/xatmi_call.h"
#include "sf/trace.h"


namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {

            template< typename M>
            sf::platform::Uuid enqueue( const queue::Lookup& lookup, M&& message)
            {
               Trace trace( "casual::queue::enqueue");

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
               request.message.type.name = message.payload.type.type;
               request.message.type.subname = message.payload.type.subtype;
               request.message.properties = message.attributes.properties;
               request.message.reply = message.attributes.reply;
               request.message.avalible = message.attributes.available;

               request.name = lookup.name();

               auto group = lookup();

               if( group.process.queue == 0)
               {
                  throw common::exception::invalid::Argument{ "failed to look up queue"};
               }
               request.queue = group.queue;

               //log << "message: " << message << '\n';

               return common::communication::ipc::call( group.process.queue, request).id;

            }


            std::vector< Message> dequeue( const queue::Lookup& lookup, const Selector& selector, bool block = false)
            {
               Trace trace{ "casual::queue::dequeue"};


               auto& transaction = common::transaction::context().current();


               casual::common::communication::ipc::Helper ipc;

               auto group = lookup();


               auto forget_blocking = common::scope::execute( [&]()
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
                  catch( const common::exception::communication::Unavailable&)
                  {
                     // queue-broker is off-line
                  }
               });

               auto send_request = [&]()
               {
                  common::message::queue::dequeue::Request request;
                  request.trid = transaction.trid;

                  request.process = common::process::handle();

                  request.queue = group.queue;
                  request.block = block;
                  request.selector.id = selector.id;
                  request.selector.properties = selector.properties;

                  log << "async::dequeue - request: " << request << std::endl;

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
                        throw common::exception::invalid::Semantic{ "correlation mismatch"};
                     }
                     common::range::transform( reply.message, result, queue::transform::Message{});



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
                  common::message::handle::Shutdown{}
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

            return std::move( local::dequeue( lookup, selector, true).at( 0));
         }
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
               Payload( T&& type_, Iter first, Iter last)
                 : data( first, last)
               {
                  type.type = type_.name;
                  type.subtype = type_.subname;
               }

               Payload( Payload&&) = default;
               Payload& operator = ( Payload&&) = default;

               queue::Payload::type_t type;

               sf::platform::binary_type data;

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
                  payload.type.name = std::move( message.payload.type.type);
                  payload.type.subname = std::move( message.payload.type.subtype);
                  payload.memory = std::move( message.payload.data);

                  result.payload.size = payload.memory.size();
                  result.payload.buffer = common::buffer::pool::Holder::instance().insert( std::move( payload));
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
         std::vector< Message> queue( const std::string& queue)
         {
            std::vector< Message> result;

            return result;
         }

      } // peek

   } // queue
} // casual
