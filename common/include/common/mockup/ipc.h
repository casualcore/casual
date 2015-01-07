//!
//! ipc.h
//!
//! Created on: May 30, 2014
//!     Author: Lazan
//!

#ifndef COMMON_MOCKUP_IPC_H_
#define COMMON_MOCKUP_IPC_H_

#include "common/ipc.h"
#include "common/move.h"
#include "common/platform.h"
#include "common/message/type.h"


namespace casual
{
   namespace common
   {
      namespace mockup
      {
         namespace ipc
         {
            using id_type = platform::queue_id_type;

            using transform_type = std::function< common::ipc::message::Complete( common::ipc::message::Complete&)>;


            //!
            //! Routes messages from it's own ipc-queue to another
            //! and caches messages if the target is full
            //!
            //!
            struct Router
            {
               //!
               //! @param destination where to send messages
               //! @param transform invoked before send, hence one can transform
               //!   complete messages (mostly to transform regest->reply and keep correlation)
               //!
               Router( id_type destination, transform_type transform);
               Router( id_type destination);

               template< typename D, typename... Args>
               Router( D&& destination, Args&&... args) : Router( destination.id(), std::forward< Args>( args)...) {}

               ~Router();

               id_type id() const;
               id_type destination() const;

            private:
               class Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };


            //!
            //! Acts as an instance.
            //!
            //! In a separate thread:
            //!  - consumes from ipc-queue denoted by @p id()
            //!  - writes to ipc-queue denoted by @p receive()
            //!
            //! The source queue @p id() will always be writable
            //!
            //! messages can be read from @p receive()
            //!
            struct Instance
            {
               Instance( platform::pid_type pid);

               //!
               //! sets current process pid
               //!
               Instance();
               ~Instance();

               Instance( Instance&&) noexcept;
               Instance& operator = ( Instance&&) noexcept;

               platform::pid_type pid() const;

               id_type id() const;

               common::process::Handle server() const;

               common::ipc::receive::Queue& receive();

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
