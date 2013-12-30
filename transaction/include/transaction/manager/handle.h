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




            struct Prepare : public state::Base
            {
               typedef common::message::transaction::resource::prepare::Reply message_type;

               using state::Base::Base;

               void dispatch( message_type& message);

            };

            struct Commit : public state::Base
            {
               typedef common::message::transaction::resource::commit::Reply message_type;

               using state::Base::Base;

               void dispatch( message_type& message);

            };

            struct Rollback : public state::Base
            {
               typedef common::message::transaction::resource::rollback::Reply message_type;

               using state::Base::Base;

               void dispatch( message_type& message);

            };


         } // resource


         struct Begin : public state::Base
         {
            typedef common::message::transaction::begin::Request message_type;

            using Base::Base;

            void dispatch( message_type& message);
         };

         struct Commit : public state::Base
         {
            typedef common::message::transaction::commit::Request message_type;

            using Base::Base;

            void dispatch( message_type& message);
         };

         struct Rollback : public state::Base
         {
            typedef common::message::transaction::rollback::Request message_type;

            using Base::Base;

            void dispatch( message_type& message);
         };

         struct Involved : public state::Base
         {
            typedef common::message::transaction::resource::Involved message_type;

            using Base::Base;

            void dispatch( message_type& message);
         };


      } // handle
   } // transaction

} // casual

#endif // MANAGER_HANDLE_H_
