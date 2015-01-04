//!
//! common.cpp
//!
//! Created on: Nov 30, 2014
//!     Author: Lazan
//!

#include "queue/forward/common.h"
#include "queue/common/queue.h"

#include "common/queue.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/transaction/context.h"
#include "common/server/handle.h"

namespace casual
{
   namespace queue
   {
      namespace forward
      {
         namespace local
         {
            namespace
            {
               std::vector< Dispatch::Task> lookup( const std::vector< forward::Task>& tasks)
               {
                  std::vector< Dispatch::Task> result;

                  for( auto& task : tasks)
                  {
                     auto reply = queue::Lookup{ task.queue}();

                     if( reply.queue == 0)
                     {
                        throw common::exception::invalid::Argument{ "failed to lookup queue: " + task.queue};
                     }

                     Dispatch::Task dispatch;

                     dispatch.process = reply.process;
                     dispatch.queue.id = reply.queue;
                     dispatch.queue.name = task.queue;
                     dispatch.dispatch = task.dispatch;

                     result.push_back( std::move( dispatch));
                  }

                  return result;
               }


               bool perform( const Dispatch::Task& task)
               {
                  common::transaction::Context::instance().begin();

                  common::message::queue::dequeue::callback::Request request;
                  request.process = common::process::handle();
                  request.queue = task.queue.id;
                  request.trid = common::transaction::Context::instance().current().trid;

                  common::queue::blocking::Send send;
                  auto correlation = send( task.process.queue, request);


                  common::queue::blocking::Reader receive{ common::ipc::receive::queue()};
                  common::message::queue::dequeue::Reply reply;
                  receive( reply, correlation);

                  if( reply.message.empty())
                  {
                     common::log::internal::queue << "message empty - rollback and wait for notification" << std::endl;
                     common::transaction::Context::instance().rollback();
                     return false;
                  }

                  try
                  {
                     common::log::internal::queue << "message not empty - call dispatch" << std::endl;

                     task.dispatch( std::move( reply.message.front()));
                     common::transaction::Context::instance().commit();
                  }
                  catch( ...)
                  {
                     common::error::handler();
                     common::transaction::Context::instance().rollback();
                  }

                  return true;
               }

            } // <unnamed>
         } // local


         Dispatch::Dispatch( const std::vector< forward::Task>& tasks)
            : m_tasks( local::lookup( tasks))
         {
            if( m_tasks.size() != 1)
            {
               throw common::exception::invalid::Argument{ "only one task is allowed"};
            }

            common::signal::timer::Scoped timout{ std::chrono::seconds{ 5}};

            common::server::connect( {});

         }

         Dispatch::Dispatch( const std::vector< forward::Task>& tasks, const std::vector< common::transaction::Resource>& resources)
           : m_tasks( local::lookup( tasks))
         {
            if( m_tasks.size() != 1)
            {
               throw common::exception::invalid::Argument{ "only one task is allowed"};
            }

            common::signal::timer::Scoped timout{ std::chrono::seconds{ 5}};

            common::server::connect( {}, resources);

         }


         namespace handle
         {

            namespace callback
            {

               struct Reply
               {
                  using message_type = common::message::queue::dequeue::callback::Reply;

                  void dispatch( message_type& message)
                  {
                     //
                     // We do nothing, this just acts as a blocker, and when we get
                     // this message, we continue
                     //
                  }
               };

            } // callback


         } // handle


         void Dispatch::execute()
         {
            common::message::dispatch::Handler handler{
               handle::callback::Reply{},
               common::message::handle::Shutdown{},
               common::message::handle::ping(),
            };

            while( true)
            {
               //
               // Perform the task until there are no more messages
               //
               while( local::perform( m_tasks.front()))
               {
                  common::queue::non_blocking::Reader receive{ common::ipc::receive::queue()};
                  handler.dispatch( receive.next());
               }



               //
               // Wait to be notified when there are some messages on the queue.
               // or someone wants us to shut down...
               //
               common::queue::blocking::Reader receive{ common::ipc::receive::queue()};
               handler.dispatch( receive.next());
            }

         }

      } // forward
   } // queue
} // casual
