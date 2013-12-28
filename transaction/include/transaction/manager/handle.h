//!
//! manager_handle.h
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_HANDLE_H_
#define MANAGER_HANDLE_H_


#include "transaction/manager/state.h"

#include "common/message.h"
#include "common/log.h"
#include "common/exception.h"
#include "common/process.h"
#include "common/queue.h"
#include "common/algorithm.h"


using casual::common::exception::signal::child::Terminate;
using casual::common::process::lifetime;
using casual::common::queue::blocking::basic_reader;




namespace casual
{
   namespace transaction
   {

      namespace policy
      {
         struct Manager : public state::Base
         {
            using state::Base::Base;

            void apply()
            {
               try
               {
                  throw;
               }
               catch( const common::exception::signal::child::Terminate& exception)
               {
                  auto terminated = common::process::lifetime::ended();
                  for( auto& death : terminated)
                  {
                     switch( death.why)
                     {
                        case common::process::lifetime::Exit::Why::core:
                           common::log::error << "process crashed: TODO: maybe restart? " << death.string() << std::endl;
                           break;
                        default:
                           common::log::information << "proccess died: " << death.string() << std::endl;
                           break;
                     }

                     state::remove::instance( death.pid, m_state);
                  }
               }
            }
         };
      } // policy


      namespace queue
      {
         namespace blocking
         {
            using Reader = common::queue::blocking::basic_reader< policy::Manager>;
            using Writer = common::queue::blocking::basic_writer< policy::Manager>;

         } // blocking

         namespace non_blocking
         {
            using Reader = common::queue::non_blocking::basic_reader< policy::Manager>;
            using Writer = common::queue::non_blocking::basic_writer< policy::Manager>;

         } // non_blocking

      } // queue

      namespace handle
      {


         namespace resource
         {

            template< typename BQ>
            struct Connect : public state::Base
            {
               typedef common::message::transaction::resource::connect::Reply message_type;

               using broker_queue = BQ;

               Connect( State& state, broker_queue& brokerQueue) : state::Base( state), m_brokerQueue( brokerQueue) {}

               void dispatch( message_type& message)
               {
                  common::log::information << "resource proxy pid: " <<  message.id.pid << " connected" << std::endl;

                  auto instanceRange = state::find::instance(
                        common::range::make( m_state.instances),
                        message);

                  if( ! instanceRange.empty())
                  {
                     if( message.state == XA_OK)
                     {
                        instanceRange.first->state = state::resource::Proxy::Instance::State::idle;
                        instanceRange.first->server = std::move( message.id);

                     }
                     else
                     {
                        common::log::error << "resource proxy pid: " <<  message.id.pid << " startup error" << std::endl;
                        instanceRange.first->state = state::resource::Proxy::Instance::State::startupError;
                        //throw common::exception::signal::Terminate{};
                        // TODO: what to do?
                     }
                  }
                  else
                  {
                     common::log::error << "transaction manager - unexpected resource connecting - pid: " << message.id.pid << " - action: discard" << std::endl;
                  }

                  auto resources = common::range::sorted::group(
                        common::range::make( m_state.instances),
                        state::resource::Proxy::Instance::order::Id{});

                  if( ! m_connected && std::all_of( std::begin( resources), std::end( resources), state::filter::Running{}))
                  {
                     //
                     // We now have enough resource proxies up and running to guarantee consistency
                     // notify broker
                     //
                     common::message::transaction::Connected running;
                     m_brokerQueue( running);

                     m_connected = true;
                  }
               }
            private:
               broker_queue& m_brokerQueue;
               bool m_connected = false;

            };

            template< typename BQ>
            Connect< BQ> connect( State& state, BQ&& brokerQueue)
            {
               return Connect< BQ>{ state, std::forward< BQ>( brokerQueue)};
            }



            namespace instance
            {

               template< typename M>
               void state( State& state, const M& message, state::resource::Proxy::Instance::State newState)
               {
                  auto instance = state::find::instance( common::range::make( state.instances), message);

                  if( ! instance.empty())
                  {
                     instance.first->state = newState;
                  }
               }

               template< typename M>
               void done( State& state, M& message)
               {
                  auto request = common::range::find(
                        common::range::make( state.pendingRequest),
                        action::pending::resource::Request::Find{ message.resource});

                  if( ! request.empty())
                  {
                     //
                     // We got a pending request for this resource, let's oblige
                     //
                     queue::non_blocking::Writer writer{ message.id.queue_id, state};

                     if( writer.send( request.first->message))
                     {
                        common::range::erase( state.pendingRequest, request);
                     }
                     else
                     {
                        common::log::warning << "failed to send pending request to resource, although the instance reported idle" << std::endl;

                        instance::state( state, message, state::resource::Proxy::Instance::State::idle);
                     }
                  }
                  else
                  {

                     instance::state( state, message, state::resource::Proxy::Instance::State::idle);

                     // TODO: else what?
                  }

               }
            } // instance




            struct Prepare : public state::Base
            {
               typedef common::message::transaction::resource::prepare::Reply message_type;

               using state::Base::Base;

               void dispatch( message_type& message)
               {

                  //
                  // Instance is ready for more work
                  //
                  instance::done( m_state, message);

                  auto task = common::range::find(
                        common::range::make( m_state.tasks),
                        action::Task::Find( message.xid));

                  if( ! task.empty())
                  {
                     auto resource = common::range::sorted::bound(
                           common::range::make( task.first->resources),
                           action::Resource{ message.resource});

                     if( ! resource.empty())
                     {
                        resource.first->state = action::Resource::State::cPrepared;
                     }
                     // TODO: else, what to do?


                     auto state = task.first->state();

                     //
                     // Are we in a prepared state?
                     //
                     if( state >= action::Resource::State::cPrepared)
                     {

                        m_state.log.prepareCommit( task.first->xid);

                     }

                  }
                  // TODO: else, what to do?

               }

            };

            struct Commit : public state::Base
            {
               typedef common::message::transaction::resource::commit::Reply message_type;

               using state::Base::Base;

               void dispatch( message_type& message)
               {
                  //
                  // Instance is ready for more work
                  //
                  instance::done( m_state, message);

               }

            };

            struct Rollback : public state::Base
            {
               typedef common::message::transaction::resource::rollback::Reply message_type;

               using state::Base::Base;

               void dispatch( message_type& message)
               {
                  //
                  // Instance is ready for more work
                  //
                  instance::done( m_state, message);

               }

            };


         } // resource


         struct Begin : public state::Base
         {
            typedef common::message::transaction::begin::Request message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               if( message.xid.null())
               {
                  message.xid.generate();
               }

               auto task = common::range::find( common::range::make( m_state.tasks), action::Task::Find{ message.xid});

               if( task.empty())
               {
                  //auto task = state::

                  m_state.log.begin( message);

                  action::pending::Reply reply;
                  reply.target = message.id.queue_id;


                  m_state.pendingReplies.push_back( std::move( reply));

               }
               else
               {
                  common::log::error << "Attempt to start a transaction " << message.xid.stringGlobal() << ", which is already in progress" << std::endl;
                  // TODO: send reply
               }


            }
         };

         struct Commit : public state::Base
         {
            typedef common::message::transaction::commit::Request message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               //
               // Find the task
               //
               auto task = common::range::find( common::range::make( m_state.tasks), action::Task::Find{ message.xid});


               if( ! task.empty())
               {

                  //
                  // Prepare prepare-requests
                  //
                  action::pending::transform::Request< common::message::transaction::resource::prepare::Request> transform{ message.xid};

                  common::range::transform(
                        common::range::make( task.first->resources),
                        m_state.pendingRequest,
                        transform);

               }
               else
               {
                  common::log::error << "Attempt to commit a transaction " << message.xid.stringGlobal() << ", which is not known to TM - action: error reply" << std::endl;

               }
            }
         };

         struct Rollback : public state::Base
         {
            typedef common::message::transaction::rollback::Request message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {

            }
         };

         struct Involved : public state::Base
         {
            typedef common::message::transaction::resource::Involved message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               auto task = common::range::find( common::range::make( m_state.tasks), action::Task::Find( message.xid));

               if( ! task.empty())
               {
                  common::range::copy(
                     common::range::make( message.resources),
                     std::back_inserter( task.first->resources));

                  common::range::trim( task.first->resources, common::range::unique( common::range::sort( common::range::make( task.first->resources))));

               }
               else
               {
                  common::log::error << "Resource " << common::range::make( message.resources) << " claims to be involved in transaction " << message.xid.stringGlobal() << ", which is not known to TM - action: discard" << std::endl;
               }
            }
         };


      } // handle
   } // transaction

} // casual

#endif // MANAGER_HANDLE_H_
