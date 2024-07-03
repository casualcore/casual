//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "queue/group/queuebase.h"
#include "queue/common/ipc/message.h"

#include "casual/platform.h"

#include "common/state/machine.h"
#include "common/communication/select.h"
#include "common/communication/ipc/send.h"
#include "common/communication/ipc/pending.h"

#include <string>

namespace casual
{
   namespace queue::group
   {
      namespace state
      {
         struct Pending
         {
            template< typename M>
            void reply( M&& message, const common::process::Handle& destination)
            {
               replies.add( destination, std::forward< M>( message));
            }

            void add( ipc::message::group::dequeue::Request&& message);

            ipc::message::group::dequeue::forget::Reply forget( const ipc::message::group::dequeue::forget::Request& message);

            //! forgets all pending dequeue request.
            //! @returns forget request that should be sent to pending callers
            common::communication::ipc::pending::Holder forget();

            //! @returns dequeue-request that potentially has messages available for dequeue
            std::vector< ipc::message::group::dequeue::Request> extract( std::vector< common::strong::queue::id> queues);

            void remove( common::strong::process::id pid);
            void remove( common::strong::ipc::id ipc);

            inline auto empty() const noexcept { return replies.empty() && dequeues.empty();}

            common::communication::ipc::pending::Holder replies;
            std::vector< ipc::message::group::dequeue::Request> dequeues;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( replies);
               CASUAL_SERIALIZE( dequeues);
            )
         };

         enum struct Runlevel : short
         {
            running,
            shutdown,
            error,
         };
         std::string_view description( Runlevel value);
         
      } // state

      struct State
      {
         common::state::Machine< state::Runlevel> runlevel;

         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         Queuebase queuebase;
         state::Pending pending;

         //! A log to know if we already have notified TM about
         //! a given transaction.
         std::vector< common::transaction::ID> involved;

         std::vector< common::strong::queue::id> zombies;

         std::string alias;
         std::string note;

         struct
         {
            platform::size::type current;
            platform::size::type capacity;

            inline void add( platform::size::type size) { current += size;}
            inline void subtract( platform::size::type size) { current -= size;}
            inline platform::size::type available() const { return capacity - current;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( current);
               CASUAL_SERIALIZE( capacity);
            )
         } size;

         bool done() const noexcept;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( involved);
            CASUAL_SERIALIZE( zombies);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( note);
            CASUAL_SERIALIZE( size);
         )
      };
   } // queue::group
} // casual