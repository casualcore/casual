//!
//! casual
//!

#ifndef MANAGER_HANDLE_H_
#define MANAGER_HANDLE_H_


#include "transaction/manager/state.h"
#include "transaction/manager/action.h"

#include "common/message/transaction.h"
#include "common/message/domain.h"
#include "common/log.h"
#include "common/exception.h"
#include "common/process.h"
#include "common/algorithm.h"
#include "common/communication/ipc.h"

// For handle call
#include "common/transaction/context.h"
#include "common/server/context.h"


namespace casual
{
   namespace transaction
   {

      namespace ipc
      {

         const common::communication::ipc::Helper& device();

      } // ipc

      namespace user
      {

         using error = common::exception::code::category::error;

      } // transaction


      namespace handle
      {

         namespace implementation
         {
            struct Interface
            {
               virtual bool prepare( State& state, common::message::transaction::resource::prepare::Reply& message, Transaction& transaction) const = 0;
               virtual bool commit( State& state, common::message::transaction::resource::commit::Reply& message, Transaction& transaction) const = 0;
               virtual bool rollback( State& state, common::message::transaction::resource::rollback::Reply& message, Transaction& transaction) const = 0;
            };
         } // implementation


         namespace process
         {
            struct Exit : state::Base
            {
               using Base::Base;

               using message_type = common::message::domain::process::termination::Event;

               void operator () ( message_type& message);


               void apply( const common::process::lifetime::Exit& exit);
            };

         } // process


         namespace resource
         {

            //!
            //! Sent by a server when resource(s) is involved
            //!
            struct Involved : public state::Base
            {
               using Base::Base;

               void operator () ( common::message::transaction::resource::Involved& message);
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
                  using message_type = common::message::transaction::resource::prepare::Reply;

                  using state::Base::Base;

                  bool operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };
               using Prepare = Wrapper< basic_prepare>;


               struct basic_commit : public state::Base
               {
                  using message_type = common::message::transaction::resource::commit::Reply;

                  using state::Base::Base;

                  bool operator () ( message_type& message, Transaction& transaction, Transaction::Resource& resource);

               };
               using Commit = Wrapper< basic_commit>;

               struct basic_rollback : public state::Base
               {
                  using message_type = common::message::transaction::resource::rollback::Reply;

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
            using message_type = common::message::transaction::commit::Request;
            using reply_type = common::message::transaction::commit::Reply;

            using Base::Base;

            void operator () ( message_type& message);
         };

         using Commit = user_reply_wrapper< basic_commit>;


         struct basic_rollback : public state::Base
         {
            using message_type = common::message::transaction::rollback::Request;
            using reply_type = common::message::transaction::rollback::Reply;

            using Base::Base;

            void operator () ( message_type& message);
         };

         using Rollback = user_reply_wrapper< basic_rollback>;


         namespace external
         {
            struct Involved : public state::Base
            {
               using Base::Base;

               void operator () ( common::message::transaction::resource::external::Involved& message);
            };
         }


         //!
         //! This is used when this TM act as an resource to
         //! other TM:s, as in other domains.
         //!
         namespace domain
         {


            struct Prepare : public state::Base
            {
               using message_type = common::message::transaction::resource::prepare::Request;
               using reply_type = common::message::transaction::resource::prepare::Reply;

               using Base::Base;

               void operator () ( message_type& message);
            private:
               bool handle( message_type& message, Transaction& transaction);
            };

            struct Commit : public state::Base
            {
               using message_type = common::message::transaction::resource::commit::Request;
               using reply_type = common::message::transaction::resource::commit::Reply;

               using state::Base::Base;

               void operator () ( message_type& message);

            private:
               void handle( message_type& message, Transaction& transaction);
            };

            struct Rollback : public state::Base
            {
               using message_type = common::message::transaction::resource::rollback::Request;
               using reply_type = common::message::transaction::resource::rollback::Reply;

               using state::Base::Base;

               void operator () ( message_type& message);

            };

         } // domain

      } // handle
   } // transaction
} // casual


#endif // MANAGER_HANDLE_H_
