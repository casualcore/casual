//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/event.h"

#include "common/message/pending.h"
#include "common/communication/ipc/message.h"
#include "common/serialize/native/complete.h"
#include "common/algorithm/container.h"
#include "common/stream.h"

namespace casual
{
   namespace common::event
   {
      template< typename Event>
      struct Dispatch 
      {
         void subscription( const message::event::subscription::Begin& message)
         {
            if( ( message.types.empty() || algorithm::find( message.types, Event::type())) && ! exists( message.process))
               m_subscribers.push_back( message.process);
         }

         void subscription( const message::event::subscription::End& message)
         {
            remove( message.process);
         }

         //! remove subscribers with pid or ipc
         template< typename ID>
         void remove( ID id)
         {
            algorithm::container::trim( m_subscribers, algorithm::remove( m_subscribers, id));
         }

         explicit operator bool () const { return ! m_subscribers.empty();}

         template< typename ID>
         bool exists( ID id) const
         {
            return predicate::boolean( algorithm::find( m_subscribers, id));
         }

         //! @deprecated
         common::message::pending::Message operator ()( const Event& event) const
         {
            return common::message::pending::Message{
               serialize::native::complete< communication::ipc::message::Complete>( event),
               m_subscribers};
         }

         template< typename MS>
         void operator ()( MS& multiplex, const Event& event) const
         {
            for( auto subscriber : m_subscribers)
               multiplex.send( subscriber.ipc, event);
         }

         inline auto& subscribers() const { return m_subscribers;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( Event::type(), "event");
            CASUAL_SERIALIZE_NAME( m_subscribers, "subscribers");
         )
      private:
         std::vector< common::process::Handle> m_subscribers;
      };

      namespace dispatch
      {
         template< typename... Events>
         struct Collection : event::Dispatch< Events>...
         {
            using event::Dispatch< Events>::operator()...;

            template< typename Event>
            event::Dispatch< Event>& event()
            {
               return static_cast< event::Dispatch< Event>&>( *this);
            }

            template< typename Event>
            const event::Dispatch< Event>& event() const
            {
               return static_cast< const event::Dispatch< Event>&>( *this);
            }

            template< typename Event>
            bool active() const
            {
               return predicate::boolean( event< Event>());
            }

            void subscription( const message::event::subscription::Begin& message)
            {
               ( ... , event< Events>().subscription( message) );
            }

            void subscription( const message::event::subscription::End& message)
            {
               ( ... , event< Events>().subscription( message) );
            }

            void remove( strong::process::id pid)
            {
               ( ... , event< Events>().remove( pid) );
            }

            friend std::ostream& operator << ( std::ostream& out, const Collection& value)
            {
               out << "{ ";
               value.print( out, Events{}...);
               return out << '}';
            }

         private:

            template< typename E>
            void print( std::ostream& out, E&&) const
            {
               stream::write( out, event< E>());
            }

            template< typename E, typename... Es>
            void print( std::ostream& out, E&&, Es&&...) const
            {
               stream::write( out, event< E>(), ", ");
               print( out, Es{}...);
            }

         };
      } // dispatch
   } // common::event
} // casual


