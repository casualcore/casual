//!
//! casual
//!

#include "queue/group/group.h"
#include "queue/group/handle.h"

#include "queue/common/log.h"
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
            Trace trace{ "queue::group::State::Pending::dequeue"};

            log << "request: " << request << '\n';

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
               requests.erase( std::begin( found));

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
               transactions.erase( std::begin( found));

               //
               // Move all request that is interested in committed queues to the end.
               // We use the second part of the partition directly
               //
               auto interested = std::get< 1>( common::range::stable_partition( requests, [&]( const request_type& r){
                  return ! common::range::find( result.enqueued, r.queue);
               }));

               //
               // move the requests to the result, and erase them in pending
               //
               common::range::move( interested, result.requests);
               requests.erase( std::begin( interested), std::end( interested));
            }

            return result;
         }

         void State::Pending::rollback( const common::transaction::ID& trid)
         {
            transactions.erase( trid);
         }

         void State::Pending::erase( common::strong::process::id pid)
         {
            auto found = std::get< 1>( common::range::stable_partition( requests, [=]( const request_type& r){
               return r.process.pid != pid;
            }));

            requests.erase( std::begin( found), std::end( found));
         }


         namespace message
         {
            void pump( group::State& state)
            {
               auto handler = group::handler( state);

               common::communication::ipc::Helper ipc;

               while( true)
               {
                  {
                     auto persistent = sql::database::scoped::write( state.queuebase);


                     if( state.persistent.empty())
                     {
                        //
                        // We can only block if our back log is empty...
                        //

                        handler( ipc.blocking_next());
                     }

                     //
                     // Consume until the queue is empty or we've got pending replies equal to transaction_batch
                     //

                     while( handler( ipc.non_blocking_next()) &&
                           state.persistent.size() < common::platform::batch::queue::persitent)
                     {
                        ;
                     }
                  }

                  //
                  // queuebase is persistent - send pending persistent replies
                  //
                  common::range::trim( state.persistent, common::range::remove_if(
                     state.persistent,
                     common::message::pending::sender( common::communication::ipc::policy::non::Blocking{})));

               }

            }
         } // message


         Server::Server( Settings settings) : m_state( std::move( settings.queuebase), std::move( settings.name))
         {
            //
            // Talk to queue-manager to get configuration
            //


            {
               common::message::queue::connect::Request request;
               request.process = common::process::handle();

               common::communication::ipc::blocking::send( common::communication::ipc::queue::manager::device(), request);
            }

            {
               std::vector< std::string> existing;
               for( auto&& queue : m_state.queuebase.queues())
               {
                  existing.push_back( queue.name);
               }


               common::message::queue::connect::Reply reply;
               common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);

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
               // Send all our queues to queue-manager
               //
               {
                  common::message::queue::Information information;
                  information.process = common::process::handle();
                  information.queues = m_state.queuebase.queues();

                  common::communication::ipc::blocking::send( common::communication::ipc::queue::manager::device(), information);
               }
            }
         }

         Server::~Server()
         {
            Trace trace{ "queue::server::Server dtor"};

            try
            {
               handle::shutdown( m_state);
            }
            catch( ...)
            {
               common::exception::handle();
            }
         }


         int Server::start() noexcept
         {
            try
            {
               message::pump( m_state);
            }
            catch( ...)
            {
               return common::exception::handle();
            }
            return 0;
         }
      } // server

   } // queue

} // casual
