//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "transaction/manager/state.h"
#include "transaction/manager/action.h"

#include "common/message/transaction.h"
#include "common/message/domain.h"
#include "common/message/event.h"
#include "common/message/dispatch.h"
#include "common/log.h"
#include "common/process.h"
#include "common/algorithm.h"
#include "common/communication/ipc.h"


// for handle call
#include "common/transaction/context.h"
#include "common/server/context.h"


namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace ipc
         {
            common::communication::ipc::inbound::Device& device();
         } // ipc


         namespace handle
         {
            namespace persist
            {
               //! persist and send pending replies, if any
               void send( State& state);
            } // persist

            using dispatch_type = decltype( common::message::dispatch::handler( ipc::device()));

            namespace process
            {
               struct Exit : state::Base
               {
                  using Base::Base;
                  void operator () ( common::message::event::process::Exit& message);
               };

               void exit( const common::process::lifetime::Exit& exit);

            } // process

            namespace resource
            {

               //! Sent by a server when resource(s) is involved
               struct Involved : public state::Base
               {
                  using Base::Base;

                  void operator () ( common::message::transaction::resource::involved::Request& message);
               };

               namespace reply
               {

                  template< typename H>
                  struct Wrapper : public state::Base
                  {
                     using handler_type = H;
                     using message_type = typename handler_type::message_type;

                     Wrapper( State& state) : state::Base{ state}, m_handler{ state} {}

                     void operator () ( message_type& message);


                  private:
                     H m_handler;
                  };


                  struct basic_prepare : public state::Base
                  {
                     using message_type = common::message::transaction::resource::prepare::Reply;

                     using state::Base::Base;

                     bool operator () ( message_type& message, Transaction& transaction);

                     static constexpr Transaction::Resource::Stage stage() { return Transaction::Resource::Stage::prepare_replied;}

                  };
                  using Prepare = Wrapper< basic_prepare>;


                  struct basic_commit : public state::Base
                  {
                     using message_type = common::message::transaction::resource::commit::Reply;

                     using state::Base::Base;

                     bool operator () ( message_type& message, Transaction& transaction);

                     static constexpr Transaction::Resource::Stage stage() { return Transaction::Resource::Stage::commit_replied;}

                  };
                  using Commit = Wrapper< basic_commit>;

                  struct basic_rollback : public state::Base
                  {
                     using message_type = common::message::transaction::resource::rollback::Reply;

                     using state::Base::Base;

                     bool operator () ( message_type& message, Transaction& transaction);

                     static constexpr Transaction::Resource::Stage stage() { return Transaction::Resource::Stage::rollback_replied;}
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

            //! This is used when this TM act as an resource to
            //! other TM:s, as in other domains.
            namespace domain
            {
               enum class Directive : char
               {
                  keep_transaction,
                  remove_transaction,
               };

               struct Base : state::Base
               {
                  using state::Base::Base;

               protected:
                  template< typename M>
                  void prepare_remote_owner( Transaction& transaction, M& message);
               };


               struct Prepare : Base
               {
                  using message_type = common::message::transaction::resource::prepare::Request;
                  using reply_type = common::message::transaction::resource::prepare::Reply;

                  using Base::Base;

                  void operator () ( message_type& message);
               private:
                  Directive handle( message_type& message, Transaction& transaction);
               };

               struct Commit : Base
               {
                  using message_type = common::message::transaction::resource::commit::Request;
                  using reply_type = common::message::transaction::resource::commit::Reply;

                  using Base::Base;

                  void operator () ( message_type& message);

               private:
                  Directive handle( message_type& message, Transaction& transaction);
               };

               struct Rollback : Base
               {
                  using message_type = common::message::transaction::resource::rollback::Request;
                  using reply_type = common::message::transaction::resource::rollback::Reply;

                  using Base::Base;

                  void operator () ( message_type& message);

               private:
                  Directive handle( message_type& message, Transaction& transaction);

               };

            } // domain

            namespace startup
            {
               //! return the handlers used during startup
               dispatch_type handlers( State& state);
            } // startup

            dispatch_type handlers( State& state);

            void abort( State& state);
            

         } // handle
      } // manager
   } // transaction
} // casual



