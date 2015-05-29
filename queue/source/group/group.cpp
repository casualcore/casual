//!
//! queue.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/group/group.h"
#include "queue/group/handle.h"

#include "queue/common/environment.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"


namespace casual
{
   namespace queue
   {

      namespace group
      {

         void State::Pending::dequeue( const common::message::queue::dequeue::Request& request)
         {
            if( request.block)
            {
               requests.push_back( request);
            }
         }


         void State::Pending::enqueue( const common::transaction::ID& trid, queue_id_type id)
         {
            auto found = common::range::find_if( requests, [&]( const request_type& r){
               return r.queue == id;
            });

            if( found)
            {
               //
               // Someone is waiting for messages in this queue
               //
               transactions[ trid][ id]++;
            }
         }

         common::message::queue::dequeue::forget::Reply State::Pending::forget( const common::message::queue::dequeue::forget::Request& request)
         {
            common::message::queue::dequeue::forget::Reply reply;
            reply.correlation = request.correlation;

            auto found = common::range::find_if( requests, [&]( const request_type& r){
               return r.queue == request.queue && r.process == request.process;
            });

            reply.found = static_cast< bool>( found);

            if( found)
            {
               requests.erase( found.first);

               if( requests.empty())
               {
                  transactions.clear();
               }
            }
            return reply;
         }

         State::Pending::result_t State::Pending::commit( const common::transaction::ID& trid)
         {
            result_t result;

            auto found = common::range::find( transactions, trid);

            if( found)
            {
               result.enqueued = std::move( found->second);
               transactions.erase( found.first);

               //
               // Move all request that is interested in committed queues to the end.
               //
               auto split = common::range::stable_partition( requests, [&]( const request_type& r){
                  return ! common::range::find( result.enqueued, r.queue);
               });

               //
               // move the requests to the result, and erase them in pending
               //
               common::range::move( std::get< 1>( split), result.requests);
               requests.erase( std::begin( std::get< 1>( split)), std::end( std::get< 1>( split)));
            }

            return result;
         }

         void State::Pending::rollback( const common::transaction::ID& trid)
         {
            transactions.erase( trid);
         }

         Server::Server( Settings settings) : m_state( std::move( settings.queuebase), std::move( settings.name))
         {
            //
            // Talk to queue-broker to get configuration
            //

            group::queue::blocking::Writer queueBroker{ environment::broker::queue::id(), m_state};

            {
               common::message::queue::connect::Request request;
               request.process = common::process::handle();
               queueBroker( request);
            }

            {
               std::vector< std::string> existing;
               for( auto&& queue : m_state.queuebase.queues())
               {
                  existing.push_back( queue.name);
               }



               group::queue::blocking::Reader read( common::ipc::receive::queue(), m_state);
               common::message::queue::connect::Reply reply;
               read( reply);

               std::vector< std::string> added;

               for( auto&& queue : reply.queues)
               {
                  auto exists = common::range::find( existing, queue.name);

                  if( ! exists)
                  {
                     m_state.queuebase.create( Queue{ queue.name, queue.retries});
                     added.push_back( queue.name);
                  }
               }


               //
               // Try to remove queues
               // TODO:
               //
               //auto removed = common::range::difference( existing, added);


               //
               // Send all our queues to queue-broker
               //
               common::message::queue::Information information;
               information.process = common::process::handle();
               information.queues = m_state.queuebase.queues();

               queueBroker( information);
            }
         } // group


         void Server::start()
         {
            common::message::dispatch::Handler handler{
               handle::enqueue::Request{ m_state},
               handle::dequeue::Request{ m_state},
               handle::dequeue::forget::Request{ m_state},
               handle::transaction::commit::Request{ m_state},
               handle::transaction::rollback::Request{ m_state},
               handle::information::queues::Request{ m_state},
               handle::information::messages::Request{ m_state},
               common::message::handle::Shutdown{},
            };


            group::queue::blocking::Reader blockedRead( common::ipc::receive::queue(), m_state);

            while( true)
            {
               {
                  auto persistent = sql::database::scoped::write( m_state.queuebase);


                  handler( blockedRead.next());

                  //
                  // Consume until the queue is empty or we've got pending replies equal to transaction_batch
                  //

                  group::queue::non_blocking::Reader nonBlocking( common::ipc::receive::queue(), m_state);

                  while( handler( nonBlocking.next()) &&
                        m_state.persistent.size() < common::platform::batch::transaction)
                  {
                     ;
                  }
               }

               //
               // queuebase is persistent - send pending persistent replies
               //
               group::queue::non_blocking::Send send{ m_state};

               auto remain = common::range::remove_if(
                  m_state.persistent,
                  common::message::pending::sender( send));

               m_state.persistent.erase( remain.last, std::end( m_state.persistent));


            }

         }
      } // server

   } // queue

} // casual
