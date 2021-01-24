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
            void reply( M&& message, const common::process::Handle& destinations)
            {
               replies.emplace_back( std::forward< M>( message), destinations);
            }

            void add( ipc::message::group::dequeue::Request&& message);

            ipc::message::group::dequeue::forget::Reply forget( const ipc::message::group::dequeue::forget::Request& message);

            //! forgets all pending dequeue request.
            //! @returns forget request that should be sent to pending callers
            std::vector< common::message::pending::Message> forget();

            //! @returns dequeue-request that potentially has messages available for dequeue
            std::vector< ipc::message::group::dequeue::Request> extract( std::vector< common::strong::queue::id> queues);

            void remove( common::strong::process::id pid);

            inline auto empty() const noexcept { return replies.empty() && dequeues.empty();}

            std::vector< common::message::pending::Message> replies;
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
         std::ostream& operator << ( std::ostream& out, Runlevel value);
         
      } // state

      struct State
      {
         inline State() = default;
         inline State( std::string filename)
            : queuebase{ std::move( filename)}
         {}

         State( State&&) = default;
         State& operator = ( State&&) = default;

         common::state::Machine< state::Runlevel> runlevel;
         Queuebase queuebase;
         state::Pending pending;

         //! A log to know if we already have notified TM about
         //! a given transaction.
         std::vector< common::transaction::ID> involved;

         std::string alias;
         std::string note;

         bool done() const noexcept;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( involved);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( note);
         )
      };
   } // queue::group
} // casual