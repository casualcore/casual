//!
//! manager_handle.h
//!
//! Created on: Aug 13, 2013
//!     Author: Lazan
//!

#ifndef MANAGER_HANDLE_H_
#define MANAGER_HANDLE_H_


#include "transaction/manager/state.h"
#include "transaction/manager/action.h"

#include "common/message/transaction.h"
#include "common/log.h"
#include "common/exception.h"
#include "common/process.h"
#include "common/queue.h"
#include "common/algorithm.h"

// For handle call
#include "common/transaction/context.h"
#include "common/server/context.h"


namespace casual
{
   namespace transaction
   {

      namespace user
      {

         using error = common::exception::code::category::error;

      } // transaction

      namespace queue
      {

         namespace blocking
         {
            using Reader = common::queue::blocking::callback::basic_reader;
            using Send = common::queue::blocking::callback::basic_send;

         } // blocking

         namespace non_blocking
         {
            using Reader = common::queue::non_blocking::callback::basic_reader;
            using Send = common::queue::non_blocking::callback::basic_send;

         } // non_blocking
      } // queue

      namespace handle
      {
         namespace dead
         {
            struct Process : state::Base
            {
               using Base::Base;

               void operator() ( const common::message::dead::process::Event& message);
            };
         } // dead

         namespace resource
         {
            namespace id
            {
               //!
               //! Sent from resource proxies to require a rm-id.
               //!
               struct Allocation : public state::Base
               {
                  typedef common::message::transaction::resource::id::Request message_type;

                  using Base::Base;

                  void operator () ( message_type& message);

               };

            } // id

            //!
            //! Sent by a server when resource(s) is involved
            //!
            struct Involved : public state::Base
            {
               typedef common::message::transaction::resource::Involved message_type;

               using Base::Base;

               void operator () ( message_type& message);
            };

            namespace reply
            {

               template< typename H>
               struct Wrapper : public state::Base
               {
                  using message_type = typename H::message_type;

                  Wrapper( State& state) : state::Base{ state}, m_handler{ state} {}

                  void operator () ( message_type& message);


               private:
                  H m_handler;
               };

               struct Connect : public state::Base
               {
                  typedef common::message::transaction::resource::connect::Reply message_type;

                  using state::Base::Base;

                  void operator () ( message_type& message);

               private:
                  bool m_connected = false;

               };


               struct basic_prepare : public state::Base
               {
                  typedef common::message::transaction::resource::prepare::Reply message_type;

                  using state::Base::Base;

                  bool operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };
               using Prepare = Wrapper< basic_prepare>;


               struct basic_commit : public state::Base
               {
                  typedef common::message::transaction::resource::commit::Reply message_type;

                  using state::Base::Base;

                  bool operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };
               using Commit = Wrapper< basic_commit>;

               struct basic_rollback : public state::Base
               {
                  typedef common::message::transaction::resource::rollback::Reply message_type;

                  using state::Base::Base;

                  bool operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };

               using Rollback = Wrapper< basic_rollback>;


            } // reply

         } // resource

         template< typename Handler>
         struct user_reply_wrapper : Handler
         {
            using Handler::Handler;

            void operator () ( typename Handler::message_type& message);
         };


         struct basic_commit : public state::Base
         {
            typedef common::message::transaction::commit::Request message_type;
            typedef common::message::transaction::commit::Reply reply_type;

            using Base::Base;

            void operator () ( message_type& message);
         };

         using Commit = user_reply_wrapper< basic_commit>;


         struct basic_rollback : public state::Base
         {
            typedef common::message::transaction::rollback::Request message_type;
            typedef common::message::transaction::rollback::Reply reply_type;

            using Base::Base;

            void operator () ( message_type& message);
         };

         using Rollback = user_reply_wrapper< basic_rollback>;


         //!
         //! This is used when this TM act as an resource to
         //! other TM:s, as in other domains.
         //!
         namespace domain
         {
            struct Involved : public state::Base
            {
               typedef common::message::transaction::resource::domain::Involved message_type;

               using Base::Base;

               void operator () ( message_type& message);
            };

            struct Prepare : public state::Base
            {
               typedef common::message::transaction::resource::prepare::Request message_type;
               typedef common::message::transaction::resource::prepare::Reply reply_type;

               using Base::Base;

               void operator () ( message_type& message);
            };

            struct Commit : public state::Base
            {
               typedef common::message::transaction::resource::commit::Request message_type;
               typedef common::message::transaction::resource::commit::Reply reply_type;

               using state::Base::Base;

               void operator () ( message_type& message);
            };

            struct Rollback : public state::Base
            {
               typedef common::message::transaction::resource::rollback::Request message_type;
               typedef common::message::transaction::resource::rollback::Reply reply_type;

               using state::Base::Base;

               void operator () ( message_type& message);

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

                      bool operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource);
                   };

                   using Prepare = handle::resource::reply::Wrapper< basic_prepare>;


                   struct basic_commit : public state::Base
                   {
                      typedef common::message::transaction::resource::domain::commit::Reply message_type;

                      using state::Base::Base;

                      bool operator () (  message_type& message, Transaction& transaction, Transaction::Resource& resource);
                   };

                   using Commit = handle::resource::reply::Wrapper< basic_commit>;


                   struct basic_rollback : public state::Base
                   {
                      typedef common::message::transaction::resource::domain::rollback::Reply message_type;

                      using state::Base::Base;

                      bool operator () (  message_type& message, Transaction& transaction, Transaction::Resource& resource);

                   };

                   using Rollback = handle::resource::reply::Wrapper< basic_rollback>;

               } // reply

            } // resource

         } // domain

      } // handle
   } // transaction
} // casual


#endif // MANAGER_HANDLE_H_
