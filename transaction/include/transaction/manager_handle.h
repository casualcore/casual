//!
//! manager_handle.h
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_HANDLE_H_
#define MANAGER_HANDLE_H_


#include "transaction/manager_state.h"

#include "common/message.h"
#include "common/logger.h"
#include "common/exception.h"
#include "common/process.h"
#include "common/queue.h"

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
                           common::logger::error << "process crashed: TODO: maybe restart? " << death.string();
                           break;
                        default:
                           common::logger::information << "proccess died: " << death.string();
                           break;
                     }

                     state::remove::instance( death.pid, m_state);
                  }
               }
            }
         };
      } // policy


      using QueueBlockingReader = common::queue::blocking::basic_reader< policy::Manager>;
      using QueueNonBlockingReader = common::queue::non_blocking::basic_reader< policy::Manager>;

      using QueueBlockingWriter = common::queue::ipc_wrapper< common::queue::blocking::basic_writer< policy::Manager>>;
      //using QueueNonBlockingWriter = common::queue::ipc_wrapper< common::queue::non_blocking::basic_writer< policy::Manager>>;

      namespace handle
      {



         struct ResourceConnect : public state::Base
         {
            typedef common::message::transaction::reply::resource::Connect message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               auto instance = m_state.instances.find( message.id.pid);

               if( instance != std::end( m_state.instances))
               {
                  common::logger::debug << "transaction manager pid: " <<  message.id.pid << " connected";

                  if( message.state == XA_OK)
                  {
                     instance->second->state = state::resource::Proxy::Instance::State::idle;
                     instance->second->id = std::move( message.id);

                  }
                  else
                  {
                     instance->second->state = state::resource::Proxy::Instance::State::startupError;
                     // TODO: what to do?
                  }

               }
               else
               {
                  common::logger::error << "transaction manager - unexpected resource connecting - pid: " << message.id.pid << " - action: discard";
               }
            }

         };

         struct Begin : public state::Base
         {
            typedef common::message::transaction::Begin message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {
               long state = 0;
               auto started = std::chrono::time_point_cast<std::chrono::microseconds>(message.start).time_since_epoch().count();
               auto xid = common::transform::xid( message.xid);

               const std::string sql{ R"( INSERT INTO trans VALUES (?,?,?,?,?); )"};

               state::pending::Reply reply;
               reply.target = message.server.queue_id;

               m_state.db.execute( sql, std::get< 0>( xid), std::get< 1>( xid), message.server.pid, state, started);

               m_state.pendingReplies.push_back( std::move( reply));
            }
         };

         struct Commit : public state::Base
         {
            typedef common::message::transaction::Commit message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {

            }
         };

         struct Rollback : public state::Base
         {
            typedef common::message::transaction::Rollback message_type;

            using Base::Base;

            void dispatch( message_type& message)
            {

            }
         };

      } // handle
   } // transaction


} // casual

#endif // MANAGER_HANDLE_H_
