//!
//! manager_handle.h
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_HANDLE_H_
#define MANAGER_HANDLE_H_


#include "transaction/manager/state.h"

#include "common/message/transaction.h"
#include "common/log.h"
#include "common/exception.h"
#include "common/process.h"
#include "common/queue.h"
#include "common/algorithm.h"

// For handle call
#include "common/transaction_context.h"
#include "common/server_context.h"


namespace casual
{
   namespace transaction
   {

      namespace policy
      {
         struct Manager : public state::Base
         {
            using state::Base::Base;

            void apply();
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

            //!
            //! Sent by a server when resource(s) is involved
            //!
            struct Involved : public state::Base
            {
               typedef common::message::transaction::resource::Involved message_type;

               using Base::Base;

               void dispatch( message_type& message);
            };

            namespace reply
            {

               template< typename H>
               struct Wrapper : public state::Base
               {
                  using message_type = typename H::message_type;

                  Wrapper( State& state) : state::Base{ state}, m_handler{ state} {}

                  void dispatch( message_type& message);


               private:
                  H m_handler;
               };

               struct Connect : public state::Base
               {
                  typedef common::message::transaction::resource::connect::Reply message_type;

                  using state::Base::Base;

                  void dispatch( message_type& message);

               private:
                  bool m_connected = false;

               };


               struct basic_prepare : public state::Base
               {
                  typedef common::message::transaction::resource::prepare::Reply message_type;

                  using state::Base::Base;

                  bool dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };
               using Prepare = Wrapper< basic_prepare>;


               struct basic_commit : public state::Base
               {
                  typedef common::message::transaction::resource::commit::Reply message_type;

                  using state::Base::Base;

                  bool dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };
               using Commit = Wrapper< basic_commit>;

               struct basic_rollback : public state::Base
               {
                  typedef common::message::transaction::resource::rollback::Reply message_type;

                  using state::Base::Base;

                  bool dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };

               using Rollback = Wrapper< basic_rollback>;


            } // reply

         } // resource



         struct Begin : public state::Base
         {

            typedef common::message::transaction::begin::Request message_type;
            typedef common::message::transaction::begin::Reply reply_type;

            using Base::Base;

            void dispatch( message_type& message);
         };

         struct Commit : public state::Base
         {
            typedef common::message::transaction::commit::Request message_type;
            typedef common::message::transaction::commit::Reply reply_type;

            using Base::Base;

            void dispatch( message_type& message);
         };


         struct Rollback : public state::Base
         {
            typedef common::message::transaction::rollback::Request message_type;
            typedef common::message::transaction::rollback::Reply reply_type;

            using Base::Base;

            void dispatch( message_type& message);
         };


         //!
         //! This is used when this TM act as an resource to
         //! other TM:s, as in other domains.
         //!
         namespace domain
         {

            struct Prepare : public state::Base
            {
               typedef common::message::transaction::resource::prepare::Request message_type;
               typedef common::message::transaction::resource::prepare::Reply reply_type;

               using Base::Base;

               void dispatch( message_type& message);
            };

            struct Commit : public state::Base
            {
               typedef common::message::transaction::resource::commit::Request message_type;
               typedef common::message::transaction::resource::commit::Reply reply_type;

               using state::Base::Base;

               void dispatch( message_type& message);
            };

            struct Rollback : public state::Base
            {
               typedef common::message::transaction::resource::rollback::Request message_type;
               typedef common::message::transaction::resource::rollback::Reply reply_type;

               using state::Base::Base;

               void dispatch( message_type& message);

            };

            //!
            //! These handles responses from the resources in an inter-domain-context.
            //!
            namespace resource
            {

               namespace reply
               {
                   struct basic_prepare : public state::Base
                   {
                      typedef common::message::transaction::resource::domain::prepare::Reply message_type;

                      using Base::Base;

                      bool dispatch( message_type& message, Transaction& transaction, Transaction::Resource& resource);
                   };

                   using Prepare = handle::resource::reply::Wrapper< basic_prepare>;


                   struct basic_commit : public state::Base
                   {
                      typedef common::message::transaction::resource::domain::commit::Reply message_type;

                      using state::Base::Base;

                      bool dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource);
                   };

                   using Commit = handle::resource::reply::Wrapper< basic_commit>;


                   struct basic_rollback : public state::Base
                   {
                      typedef common::message::transaction::resource::domain::rollback::Reply message_type;

                      using state::Base::Base;

                      bool dispatch(  message_type& message, Transaction& transaction, Transaction::Resource& resource);

                   };

                   using Rollback = handle::resource::reply::Wrapper< basic_rollback>;

               } // reply

            } // resource

         } // domain


         namespace admin
         {
            struct Policy : public state::Base
            {
               using state::Base::Base;

               void connect( common::message::server::connect::Request& message, const std::vector< common::transaction::Resource>& resources);

               void disconnect();

               void reply( common::platform::queue_id_type id, common::message::service::Reply& message);

               void ack( const common::message::service::callee::Call& message);

               void transaction( const common::message::service::callee::Call&, const common::server::Service&)
               {
                  // No-op
               }

               void transaction( const common::message::service::Reply& message)
               {
                  // No-op
               }

               void statistics( common::platform::queue_id_type id, common::message::monitor::Notify& message)
               {
                  // No-op
               }
            };

            typedef common::callee::handle::basic_call< Policy> Call;
         } // admin

      } // handle
   } // transaction
} // casual


#endif // MANAGER_HANDLE_H_
