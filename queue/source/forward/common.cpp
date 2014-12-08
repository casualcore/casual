//!
//! common.cpp
//!
//! Created on: Nov 30, 2014
//!     Author: Lazan
//!

#include "queue/forward/common.h"
#include "queue/common/queue.h"

#include "common/queue.h"
#include "common/transaction/context.h"

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
                  request.trid = common::transaction::Context::instance().currentTransaction().trid;

                  common::queue::blocking::Send send;
                  auto correlation = send( task.process.queue, request);


                  common::queue::blocking::Reader receive{ common::ipc::receive::queue()};
                  common::message::queue::dequeue::Reply reply;
                  receive( correlation, reply);

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
         }

         void Dispatch::execute()
         {

            while( true)
            {
               //
               // Perform the task until there are no more messages
               //
               while( local::perform( m_tasks.front()))
                  ;


               //
               // Wait to be notified when there are some messages on the queue.
               //
               common::queue::blocking::Reader receive{ common::ipc::receive::queue()};
               common::message::queue::dequeue::callback::Reply notify;
               receive( notify);

            }

         }

      } // forward
   } // queue
} // casual
