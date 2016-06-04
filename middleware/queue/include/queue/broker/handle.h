//!
//! handle.h
//!
//! Created on: Jun 20, 2014
//!     Author: Lazan
//!

#ifndef QUEUE_BROKER_HANDLE_H_
#define QUEUE_BROKER_HANDLE_H_

#include "queue/broker/broker.h"

#include "common/message/queue.h"
#include "common/message/transaction.h"
#include "common/message/domain.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace queue
   {
      namespace broker
      {
         namespace handle
         {

            struct Base
            {
               Base( State& state) : m_state( state) {}

            protected:
               State& m_state;
            };

         }

         namespace ipc
         {
            const common::communication::ipc::Helper device();
         } // ipc




         namespace handle
         {
            namespace process
            {

               struct Exit : Base
               {
                  using Base::Base;

                  using message_type = common::message::domain::process::termination::Event;

                  void operator () ( message_type& message);

                  void apply( const common::process::lifetime::Exit& exit);
               };

            } // process

            namespace shutdown
            {
               struct Request : Base
               {
                  using message_type = common::message::shutdown::Request;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // shutdown


            namespace lookup
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::lookup::Request;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // lookup

            namespace connect
            {
               struct Request : Base
               {
                  using message_type = common::message::queue::Information;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // connect

            namespace group
            {
               struct Involved : Base
               {
                  using message_type = common::message::queue::group::Involved;

                  using Base::Base;

                  void operator () ( message_type& message);
               };


            }

            namespace transaction
            {
               namespace commit
               {
                  //!
                  //! Invoked from the casual-queue-rm
                  //!
                  struct Request : Base
                  {
                     using message_type = common::message::transaction::resource::commit::Request;

                     using Base::Base;

                     void operator () ( message_type& message);

                  };

                  //!
                  //! Invoked from 1..* groups
                  //!
                  struct Reply : Base
                  {
                     using message_type = common::message::transaction::resource::commit::Reply;

                     using Base::Base;

                     void operator () ( message_type& message);

                  };

               } // commit

               namespace rollback
               {
                  //!
                  //! Invoked from the casual-queue-rm
                  //!
                  struct Request : Base
                  {
                     using message_type = common::message::transaction::resource::rollback::Request;

                     using Base::Base;

                     void operator () ( message_type& message);

                  };

                  //!
                  //! Invoked from 1..* groups
                  //!
                  struct Reply : Base
                  {
                     using message_type = common::message::transaction::resource::rollback::Reply;

                     using Base::Base;

                     void operator () ( message_type& message);

                  };

               } // rollback


            } // transaction

         } // handle
      } // broker
   } // queue



} // casual

#endif // HANDLE_H_
