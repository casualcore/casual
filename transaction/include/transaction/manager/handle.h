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

         struct Policy
         {
            using block_writer = blocking::Writer;
            using non_block_writer = non_blocking::Writer;
         };

      } // queue

      namespace handle
      {


         namespace resource
         {

            namespace reply
            {

               template< typename QP, typename H>
               struct Wrapper : public state::Base
               {
                  using queue_policy = QP;
                  using message_type = typename H::message_type;

                  Wrapper( State& state) : state::Base{ state}, m_handler{ state} {}

                  void dispatch( message_type& message);


               private:
                  H m_handler;
               };


               template< typename QP>
               struct basic_connect : public state::Base
               {
                  using queue_policy = QP;
                  typedef common::message::transaction::resource::connect::Reply message_type;

                  basic_connect( State& state, common::platform::queue_id_type brokerQueueId)
                     : state::Base( state), m_brokerQueueId( brokerQueueId) {}

                  void dispatch( message_type& message);

               private:
                  common::platform::queue_id_type m_brokerQueueId;
                  bool m_connected = false;

               };

               using Connect = basic_connect< queue::Policy>;


               template< typename QP>
               struct basic_prepare : public state::Base
               {
                  typedef QP queue_policy;
                  typedef common::message::transaction::resource::prepare::Reply message_type;

                  using state::Base::Base;

                  void dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };
               using Prepare = Wrapper< queue::Policy, basic_prepare< queue::Policy>>;


               template< typename QP>
               struct basic_commit : public state::Base
               {
                  typedef QP queue_policy;
                  typedef common::message::transaction::resource::commit::Reply message_type;

                  using state::Base::Base;

                  void dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };
               using Commit = Wrapper< queue::Policy, basic_commit< queue::Policy>>;


               template< typename QP>
               struct basic_rollback : public state::Base
               {
                  typedef QP queue_policy;
                  typedef common::message::transaction::resource::rollback::Reply message_type;

                  using state::Base::Base;

                  void dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };

               using Rollback = Wrapper< queue::Policy, basic_rollback< queue::Policy>>;


            } // reply

         } // resource



         //!
         //! This is used when this TM act as an resource to
         //! other TM:s, as in other domains.
         //!
         namespace domain
         {

            template< typename QP>
            struct basic_prepare : public state::Base
            {
               typedef common::message::transaction::resource::prepare::Request message_type;
               typedef common::message::transaction::resource::prepare::Reply reply_type;

               using Base::Base;

               void dispatch( message_type& message);
            };

            using Prepare = basic_prepare< queue::Policy>;


            template< typename QP>
            struct basic_commit : public state::Base
            {
               typedef common::message::transaction::resource::commit::Request message_type;
               typedef common::message::transaction::resource::commit::Reply reply_type;

               using state::Base::Base;

               void dispatch( message_type& message);
            };

            using Commit = basic_commit< queue::Policy>;


            template< typename QP>
            struct basic_rollback : public state::Base
            {
               typedef common::message::transaction::resource::rollback::Request message_type;
               typedef common::message::transaction::resource::rollback::Reply reply_type;

               using state::Base::Base;

               void dispatch( message_type& message);

            };

            using Rollback = basic_rollback< queue::Policy>;


            //!
            //! These handles responses from the resources in an inter-domain-context.
            //!
            namespace resource
            {


            namespace reply
            {

                template< typename QP>
                struct basic_prepare : public state::Base
                {
                   typedef QP queue_policy;
                   typedef common::message::transaction::resource::domain::prepare::Reply message_type;

                   using Base::Base;

                   void dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource);
                };

                using Prepare = handle::resource::reply::Wrapper< queue::Policy, basic_prepare< queue::Policy>>;


                template< typename QP>
                struct basic_commit : public state::Base
                {
                   typedef QP queue_policy;
                   typedef common::message::transaction::resource::domain::commit::Reply message_type;

                   using state::Base::Base;

                   void dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource);
                };

                using Commit = handle::resource::reply::Wrapper< queue::Policy, basic_commit< queue::Policy>>;


                template< typename QP>
                struct basic_rollback : public state::Base
                {
                   typedef QP queue_policy;
                   typedef common::message::transaction::resource::domain::rollback::Reply message_type;

                   using state::Base::Base;

                   void dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource);

                };

                using Rollback = handle::resource::reply::Wrapper< queue::Policy, basic_rollback< queue::Policy>>;

            } // reply

            } // resource

         } // domain



         template< typename QP>
         struct basic_begin : public state::Base
         {
            typedef QP queue_policy;
            typedef common::message::transaction::begin::Request message_type;
            typedef common::message::transaction::begin::Reply reply_type;

            using Base::Base;

            void dispatch( message_type& message);
         };
         using Begin = basic_begin< queue::Policy>;


         template< typename QP>
         struct basic_commit : public state::Base
         {
            typedef QP queue_policy;
            typedef common::message::transaction::commit::Request message_type;
            typedef common::message::transaction::commit::Reply reply_type;

            using Base::Base;

            void dispatch( message_type& message);
         };

         using Commit = basic_commit< queue::Policy>;


         template< typename QP>
         struct basic_rollback : public state::Base
         {
            typedef QP queue_policy;
            typedef common::message::transaction::rollback::Request message_type;
            typedef common::message::transaction::rollback::Reply reply_type;

            using Base::Base;

            void dispatch( message_type& message);
         };

         using Rollback = basic_rollback< queue::Policy>;



         struct Involved : public state::Base
         {
            typedef common::message::transaction::resource::Involved message_type;

            using Base::Base;

            void dispatch( message_type& message);
         };


      } // handle
   } // transaction

} // casual


//
// Include the implementation
//
#include "transaction/manager/handle.hpp"




#endif // MANAGER_HANDLE_H_
