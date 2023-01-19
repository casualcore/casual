//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <utility>

#include "queue/api/queue.h"
#include "queue/common/ipc/message.h"
#include "queue/common/log.h"
#include "queue/common/queue.h"
#include "queue/manager/admin/services.h"
#include "queue/manager/admin/model.h"
#include "queue/code.h"

#include "common/range.h"
#include "common/buffer/type.h"
#include "common/buffer/pool.h"
#include "common/message/dispatch.h"
#include "common/message/dispatch/handle.h"
#include "common/transaction/context.h"
#include "common/communication/ipc.h"
#include "common/execute.h"
#include "common/array.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

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

               template< typename M>
               common::Uuid enqueue( const queue::Lookup& lookup, M&& message)
               {
                  Trace trace( "casual::queue::enqueue");
                  common::log::line( verbose::log, "message: ", message);

                  auto& transaction = common::transaction::context().current();

                  if( transaction)
                  {
                     // Make sure we trigger an interaction with the TM.
                     // Since the queue-groups act as 'external resources' to
                     // the TM
                     transaction.external();
                  }

                  ipc::message::group::enqueue::Request request;
                  request.trid = transaction.trid;

                  request.process = common::process::handle();

                  request.message.payload.data = message.payload.data;
                  request.message.payload.type = message.payload.type;
                  request.message.attributes.properties = message.attributes.properties;
                  request.message.attributes.reply = message.attributes.reply;
                  request.message.attributes.available = message.attributes.available;
                  request.name = lookup.name();

                  auto group = lookup();

                  if( ! group.process.ipc)
                     common::code::raise::error( queue::code::no_queue, "failed to lookup queue: ", lookup.name());

                  request.queue = group.queue;

                  common::log::line( verbose::log, "request: ", request);

                  auto id = common::communication::ipc::call( group.process.ipc, request).id;

                  // a 'nil' queue id indicate error...
                  if( ! id)
                     common::code::raise::error( queue::code::no_queue, "failed to lookup queue: ", lookup.name());

                  common::log::line( queue::event::log, "enqueue|", id);

                  return id;
               }

               namespace dequeue
               {
                  auto request = []( auto& lookup, auto& selector, auto& trid, auto block)
                  {
                     ipc::message::group::dequeue::Request request{ common::process::handle()};
                     request.trid = trid;
                     request.queue = lookup.queue;
                     request.block = block;
                     request.name = lookup.name;
                     request.selector.id = selector.id;
                     request.selector.properties = selector.properties;

                     common::log::line( verbose::log, "request: ", request);

                     return request;
                  };

                  namespace transform
                  {
                     auto message()
                     {
                        return []( ipc::message::group::dequeue::Message& value)
                        {
                           queue::Message result;
                           result.id = value.id;
                           result.attributes.available = value.attributes.available;
                           result.attributes.properties = std::move( value.attributes.properties);
                           result.attributes.reply = std::move( value.attributes.reply);
                           result.payload.type = std::move( value.payload.type);
                           result.payload.data = std::move( value.payload.data);
                           return result;
                        };
                     }
                  } // transform

                  namespace non
                  {
                     auto blocking( const queue::Lookup& lookup, const Selector& selector)
                     {
                        Trace trace{ "casual::queue::local::dequeue::non::blocking"};

                        auto& transaction = common::transaction::context().current();
                        auto group = lookup();

                        if( ! group)
                           common::code::raise::error( queue::code::no_queue);

                        // 'call' non-blocking dequeue
                        auto reply = common::communication::ipc::call( 
                           group.process.ipc,  
                           dequeue::request( group, selector, transaction.trid, false));

                        if( ! reply.message.empty())
                        {
                           if( transaction)
                           {
                              // Make sure we trigger an interaction with the TM.
                              // Since the queue-groups act as 'external resources' to
                              // the TM
                              transaction.external();
                           }
                           common::log::line( queue::event::log, "dequeue|", reply.message.front().id);
                        }

                        return common::algorithm::transform( reply.message, transform::message());
                     }  
                  } // non

                  auto blocking( const queue::Lookup& lookup, const Selector& selector)
                  {
                     Trace trace{ "casual::queue::local::dequeue::blocking"};

                     auto& transaction = common::transaction::context().current();
                     auto group = lookup();

                     if( ! group)
                        common::code::raise::error( queue::code::no_queue);

                     // 'request' blocking dequeue
                     auto correlation = common::communication::device::blocking::send( 
                        group.process.ipc, 
                        dequeue::request( group, selector, transaction.trid, true));

                     auto& ipc = common::communication::ipc::inbound::device();

                     struct 
                     {
                        bool done = false;
                        std::vector< Message> result;
                     } state;

                     // handles deque-reply - used for the normal reply, and also when we send dequeue::forget::Request
                     auto handle_dequeue_reply = [&]( ipc::message::group::dequeue::Reply& message)
                     {
                        Trace trace{ "casual::queue::local::dequeue::blocking handler - dequeue::Reply"};
                        common::log::line( verbose::log, "message: ", message);

                        if( ! message.message.empty())
                        {
                           if( transaction)
                           {
                              // Make sure we trigger an interaction with the TM.
                              // Since the queue-groups act as 'external resources' to
                              // the TM
                              transaction.external();
                           }

                           common::log::line( queue::event::log, "dequeue|", message.message.front().id);
                        }

                        if( message.correlation != correlation)
                           common::code::raise::error( code::system, "correlation mismatch");

                        common::algorithm::transform( message.message, state.result, transform::message());
                     };


                     auto handler = common::message::dispatch::handler( ipc,
                        // the normal case - when we get a message
                        [&]( ipc::message::group::dequeue::Reply& reply)
                        {
                           handle_dequeue_reply( reply);
                           state.done = true;
                        },
                        // queue-group want's to end the blocking dequeue for some reason
                        [&]( ipc::message::group::dequeue::forget::Request& message)
                        {
                           Trace trace{ "casual::queue::local::dequeue::blocking handler - forget::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           state.done = true;
                        },
                        // domain-manager want's us to shut down.
                        [&]( common::message::shutdown::Request& message)
                        {
                           Trace trace{ "casual::queue::local::dequeue::blocking handler - shutdown::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           // domain manager want this process to shutdown, we send forget-request
                           // AND make sure we 're-push' the shutdown request so casual can act on it later, hence
                           // actually shut down this process (when we get to the real message pump)
                           ipc.push( std::move( message));

                           // we need to send forget-dequeue
                           {
                              ipc::message::group::dequeue::forget::Request request{ common::process::handle()};
                              request.correlation = correlation;
                              request.queue = group.queue;

                              if( common::communication::device::blocking::optional::send( group.process.ipc, request))
                              {
                                 // there could be race-conditions when the queue-group has either sent the
                                 // dequeue-reply already, or instigated a dequeue::forget for some reason.
                                 auto handler = common::message::dispatch::handler( ipc,
                                    [&state]( ipc::message::group::dequeue::forget::Reply& message)
                                    {
                                       Trace trace{ "casual::queue::local::dequeue::blocking handler - forget::Request - forget::Reply"};
                                       common::log::line( verbose::log, "message: ", message);

                                       // This is either way the 'last' message from the group in this
                                       // 'session', but the two below might have been consumed.
                                       state.done = true;
                                    }, 
                                    []( ipc::message::group::dequeue::forget::Request& request) {}, // no-op
                                    [&]( ipc::message::group::dequeue::Reply& message)
                                    {
                                       Trace trace{ "casual::queue::local::dequeue::blocking handler - forget::Request - dequeue::Reply"};
                                       common::log::line( verbose::log, "message: ", message);

                                       handle_dequeue_reply( message);
                                       // we wait for the forget-reply
                                    }
                                 );

                                 common::message::dispatch::relaxed::pump(
                                    common::message::dispatch::condition::compose( 
                                       common::message::dispatch::condition::done( [&state](){ return state.done;})
                                    ),
                                    handler,
                                    ipc);
                              }
                           }

                           state.done = true;
                        }
                     );


                     // start listen on messages.
                     common::message::dispatch::relaxed::pump(
                        common::message::dispatch::condition::compose( 
                           common::message::dispatch::condition::done( [&state](){ return state.done;})
                        ),
                        handler,
                        ipc);

                     return std::move( state.result);
                  }
                  
               } // dequeue
            } // <unnamed>
         } // local

         common::Uuid enqueue( const std::string& queue, const Message& message)
         {
            Trace trace( "casual::queue::enqueue");
            
            queue::Lookup lookup( queue);

            return local::enqueue( lookup, message);
         }

         std::vector< Message> dequeue( const std::string& queue, const Selector& selector)
         {
            Trace trace{ "casual::queue::dequeue"};

            queue::Lookup lookup( queue);

            return local::dequeue::non::blocking( lookup, selector);
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
               common::log::line( verbose::log, "queue: ", queue, ", selector: ", selector);

               queue::Lookup lookup( queue);

               auto message = local::dequeue::blocking( lookup, selector);

               if( message.empty())
                  common::code::raise::error( code::no_message, "no message available");

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

                  queue::Lookup lookup( queue, ipc::message::lookup::request::context::Semantic::wait);

                  auto message = local::dequeue::blocking( lookup, selector);

                  if( message.empty())
                     common::code::raise::error( code::no_message);

                  return std::move( message.front());
               }

            } // available

         } // blocking

         namespace xatmi
         {
            namespace copy
            {
               // To hold reference data, so we don't need to copy the buffer.
               struct Payload
               {
                  template< typename T, typename Range>
                  Payload( T  type, Range range)
                    : type(std::move( type)), data( std::begin( range), std::end( range))
                  {

                  }

                  Payload( Payload&&) = default;
                  Payload& operator = ( Payload&&) = default;

                  std::string type;
                  platform::binary::type data;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( type);
                     CASUAL_SERIALIZE( data);
                  })
               };

               using Message = basic_message< Payload>;

            } // copy

            common::Uuid enqueue( const std::string& queue, const Message& message)
            {
               Trace trace{ "casual::queue::xatmi::enqueue"};

               const queue::Lookup lookup{ queue};

               auto send = common::buffer::pool::Holder::instance().get( common::buffer::handle::type{ message.payload.buffer}, message.payload.size);

               // We have to send only the real size of the buffer [buffer.begin, buffer.begin + transport_size)
               //
               // We could do this in several ways, but the most clean end ease way is to do
               // a copy... This is probably the only place we do a copy of a buffer.
               //
               // TODO: get rid of the copy in a conformant way
               //         we probably need to change the interface for 'binary' in write-archives (to take a range, or iterator first, last)
               copy::Message send_message{
                  message.id, message.attributes,
                  { send.payload().type, common::range::make( std::begin( send.payload().data), send.transport())}};

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
                     payload.data = std::move( message.payload.data);

                     auto buffer = common::buffer::pool::Holder::instance().insert( std::move( payload));
                     result.payload.buffer = std::get< 0>( buffer).underlying();
                     result.payload.size = std::get< 1>( buffer);                           
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

               ipc::message::group::message::meta::peek::Request request;
               request.name = queuename;
               request.process = common::process::handle();
               request.selector.properties = selector.properties;

               auto queue = lookup();

               if( queue.order > 0)
                  common::code::raise::error( queue::code::argument, "not possible to peek a remote queue: ", queuename);

               request.queue = queue.queue;

               auto reply = common::communication::ipc::call( queue.process.ipc, request);

               auto transform_message = []( auto& message)
               {
                  auto state = []( auto state)
                  {
                     using From = decltype( state);
                     using To = message::Information::State;
                     switch( state)
                     {
                        case From::enqueued : return To::enqueued;
                        case From::committed : return To::committed;
                        case From::dequeued : return To::dequeued;
                     }
                     // could never happend... g++ fix
                     return To::dequeued;
                  };

                  message::Information result;
                  result.id = message.id;
                  result.trid = std::move( message.trid);
                  result.state = state( message.state);
                  result.attributes.available = message.available;
                  result.attributes.reply = std::move( message.reply);
                  result.attributes.properties = std::move( message.properties);
                  result.payload.type = std::move( message.type);
                  result.payload.size = message.size;
                  result.redelivered = message.redelivered;
                  result.timestamp = message.timestamp;
                  return result;
               };

               common::algorithm::transform( reply.messages, result, transform_message);

               return result;
            }

            std::vector< Message> messages( const std::string& queuename, const std::vector< queue::Message::id_type>& ids)
            {
               Trace trace{ "casual::queue::peek::messages"};
               common::log::line( verbose::log, "queue: ", queuename, ", ids: ", ids);

               queue::Lookup lookup{ queuename};

               ipc::message::group::message::peek::Request request{ common::process::handle()};
               request.ids = ids;

               auto queue = lookup();

               if( queue.order > 0)
                  common::code::raise::error( queue::code::argument, "not possible to peek a remote queue: ", queuename);

               if( ! queue.process)
                  common::code::raise::error( queue::code::no_queue, "failed to lookup up: ", lookup.name());

               auto reply = common::communication::ipc::call( queue.process.ipc, request);

               return common::algorithm::transform( reply.messages, []( auto& message){
                  Message result;
                  result.id = message.id;
                  result.attributes.available = message.attributes.available;
                  result.attributes.reply = std::move( message.attributes.reply);
                  result.attributes.properties = std::move( message.attributes.properties);
                  result.payload.type = std::move( message.payload.type);
                  result.payload.data = std::move( message.payload.data);
                  return result;
               });
            }
         } // peek

         namespace browse
         {
             void peek( std::string name, common::unique_function< bool(Message&&)> callback)
            {
               Trace trace{ "casual::queue::browse::peek"};
               common::log::line( verbose::log, "name: ", name);

               queue::Lookup lookup{ std::move( name)};

               ipc::message::group::message::browse::Request request{ common::process::handle()};
               
               auto queue = lookup();

               if( queue.order > 0)
                  common::code::raise::error( queue::code::argument, "not possible to browse a remote queue: ", lookup.name());

               if( ! queue.process)
                  common::code::raise::error( queue::code::no_queue, "failed to lookup up: ", lookup.name());

               request.queue = queue.queue;

               while( auto reply = common::communication::ipc::call( queue.process.ipc, request))
               {
                  common::log::line( verbose::log, "reply: ", reply);

                  if( ! reply.message)
                     return;

                  // respect if the user wants to end the browsing
                  if( ! callback( local::dequeue::transform::message()( *reply.message)))
                     return;

                  // set last to move forward for the next request
                  request.last = reply.message->timestamp;
               }
            }
            
         } // browse


         namespace restore
         {
            std::vector< Affected> queue( const std::vector< std::string>& queues)
            {
               Trace trace{ "casual::queue::restore::queue"};
               common::log::line( verbose::log, "queues: ", queues);

               std::vector< Affected> affected;

               for( auto& queue : queues)
               {
                  serviceframework::service::protocol::binary::Call call;
                  call << CASUAL_NAMED_VALUE( queue);

                  auto reply = call( manager::admin::service::name::restore);

                  std::vector< queue::manager::admin::model::Affected> result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  common::algorithm::transform( result, affected, []( auto& a)
                  {
                     Affected result;
                     result.queue = std::move( a.queue.name);
                     result.count = a.count;
                     return result;
                  });
               }

               return affected;
            }
         } // restore
         namespace clear
         {
            std::vector< Affected> queue( const std::vector< std::string>& queues)
            {
               Trace trace{ "casual::queue::clear::queue"};
               common::log::line( verbose::log, "queues: ", queues);

               serviceframework::service::protocol::binary::Call call;
               call << CASUAL_NAMED_VALUE( queues);

               auto reply = call( manager::admin::service::name::clear);

               std::vector< manager::admin::model::Affected> result;
               reply >> CASUAL_NAMED_VALUE( result);

               return common::algorithm::transform( result, []( auto& a)
               {
                  Affected result;
                  result.queue = std::move( a.queue.name);
                  result.count = a.count;
                  return result;
               });
            }
         } // clear

         namespace messages
         {
            std::vector< common::Uuid> remove( const std::string& queue, const std::vector< common::Uuid>& messages)
            {
               Trace trace{ "casual::queue::messages::remove"};
               common::log::line( verbose::log, "queue: ", queue, ", messages: ", messages);

               serviceframework::service::protocol::binary::Call call;
               call << CASUAL_NAMED_VALUE( queue);
               call << CASUAL_NAMED_VALUE( messages);

               auto reply = call( manager::admin::service::name::messages::remove);

               std::vector< common::Uuid> result;
               reply >> CASUAL_NAMED_VALUE( result);
               return result;
            }
         } // messages
      } // v1
   } // queue
} // casual
