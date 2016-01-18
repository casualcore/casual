//!
//! ipc.h
//!
//! Created on: May 30, 2014
//!     Author: Lazan
//!

#ifndef COMMON_MOCKUP_IPC_H_
#define COMMON_MOCKUP_IPC_H_

//#include "common/ipc.h"
#include "common/communication/message.h"
#include "common/communication/ipc.h"
#include "common/move.h"
#include "common/platform.h"
#include "common/message/type.h"

#include "common/marshal/binary.h"

#include "common/mockup/reply.h"


namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace ipc
         {
            using id_type = platform::queue_id_type;

            using transform_type = std::function< std::vector< communication::message::Complete>( communication::message::Complete&)>;



            //!
            //! Replies to a request
            //!
            //!
            struct Replier
            {
               //!
               //! @param replier invoked on receive, and could send a reply
               //!
               Replier( reply::Handler replier);

               ~Replier();


               Replier( Replier&&) noexcept;
               Replier& operator = ( Replier&&) noexcept;

               //!
               //! input-queue is owned by the Replier
               //!
               id_type input() const;

            private:
               struct Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };



            //!
            //! Routes messages from it's own ipc-queue to another
            //! and caches messages if the target is full
            //!
            //!
            struct Router
            {
               //!
               //! @param output where to send messages
               //! @param transform invoked before send, hence one can transform
               //!   complete messages (mostly to transform request->reply and keep correlation)
               //!
               Router( id_type output, transform_type transform);
               Router( id_type output);

               ~Router();


               Router( Router&&) noexcept;
               Router& operator = ( Router&&) noexcept;



               //!
               //! input-queue is owned by the Router
               //!
               id_type input() const;

               //!
               //! output-queue is NOT owned by the router
               //!
               id_type output() const;

            private:
               class Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };

            //!
            //! Links one queue to another.
            //!
            //! Reads transport-messages from input and writes them to
            //! output. Caches transport if we can't write.
            //!
            //! neither of the input and output is owned by an instance of Link
            //!
            struct Link
            {
               Link( id_type input, id_type output);
               ~Link();

               Link( Link&&) noexcept;
               Link& operator = ( Link&&) noexcept;

               id_type input() const;
               id_type output() const;

            private:
               class Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };




            //!
            //! Acts as an instance.
            //!
            //! In a separate thread:
            //!  - consumes from ipc-queue denoted by @p input()
            //!  - apply transformation (if supplied)
            //!  - writes to ipc-queue denoted by @p output()
            //!
            //! The source queue @p input() will always be writable
            //!
            //! input- and output-queue is owned by an instance of Instance
            //!
            //! messages can be read from @p output(), or forward to another queue via Link
            //!
            struct Instance
            {
               Instance( platform::pid_type pid, transform_type transform);
               Instance( platform::pid_type pid);

               //!
               //! sets current process pid
               //!
               Instance();
               Instance( transform_type transform);
               ~Instance();

               Instance( Instance&&) noexcept;
               Instance& operator = ( Instance&&) noexcept;


               const common::process::Handle& process() const;


               id_type input() const;
               communication::ipc::inbound::Device& output();


               //!
               //! To enable the instance to act as a regular queue, non intrusive
               //!
               id_type id() const { return input();}

               //!
               //! consumes and discard all messages on @p receive()
               //!
               void clear();

            private:
               struct Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };





            namespace broker
            {
               platform::pid_type pid();

               ipc::Instance& queue();

               id_type id();

            } // broker

            namespace transaction
            {
               namespace manager
               {
                  platform::pid_type pid();

                  ipc::Instance& queue();

                  id_type id();
               } // manager

            } // transaction


            //!
            //! Clears all global mockup-ipc-queues
            //!
            void clear();

         } // ipc

      } // mockup
   } // common


} // casual

#endif // IPC_H_
