//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_DISPATCH_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_DISPATCH_H_

#include "common/message/event.h"

#include "common/communication/message.h"
#include "common/marshal/complete.h"

#include "common/message/pending.h"

namespace casual
{
   namespace common
   {
      namespace event
      {
         using type = common::message::Type;

         struct base_dispatch
         {
            void remove( strong::process::id pid);

            inline const std::vector< common::process::Handle>& subscribers() const { return m_subscribers;}

         protected:

            common::message::pending::Message pending( common::communication::message::Complete&& complete) const;

            std::vector< common::process::Handle> m_subscribers;

            bool exists( strong::ipc::id queue) const;
         };

         template< typename Event>
         struct Dispatch : base_dispatch
         {

            using message_type = common::message::Type;

            void subscription( const message::event::subscription::Begin& message)
            {
               if( ( message.types.empty() || algorithm::find( message.types, Event::type())) && ! exists( message.process.queue))
               {
                  m_subscribers.push_back( message.process);
               }
            }

            void subscription( const message::event::subscription::End& message)
            {
               algorithm::trim( m_subscribers, algorithm::remove_if( m_subscribers, [&]( auto& v){
                  return v.queue == message.process.queue;
               }));
            }

            void remove( strong::process::id pid)
            {
               algorithm::trim( m_subscribers, algorithm::remove_if( m_subscribers, [pid]( auto& v){
                  return pid == v.pid;
               }));
            }

            void subscription( strong::ipc::id queue)
            {
               if( ! exists( queue))
               {
                  m_subscribers.emplace_back( 0, queue);
               }
            }

            explicit operator bool () const { return ! m_subscribers.empty();}

            common::message::pending::Message create( const Event& event) const
            {
               return pending( marshal::complete( event));
            }

            friend std::ostream& operator << ( std::ostream& out, const Dispatch& value)
            {
               return out << "{ event: " << Event::type()
                     << ", subscribers: " << range::make( value.m_subscribers)
                     << '}';
            }

         };

         namespace dispatch
         {
            template< typename... Events>
            struct Collection : event::Dispatch< Events>...
            {

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
                  return static_cast< bool>( event< Event>());
               }

               void subscription( const message::event::subscription::Begin& message)
               {
                  do_subscription( message, Events{}...);
               }

               void subscription( const message::event::subscription::End& message)
               {
                  do_subscription( message, Events{}...);
               }

               void remove( strong::process::id pid)
               {
                  do_remove( pid, Events{}...);
               }
               
               template< typename E>
               common::message::pending::Message operator () ( const E& event) const
               {
                  // hack: gcc 5.4 thinks overloading is ambiguous, so we need to have a 
                  // level of indirection, and an explicit call.
                  return event::Dispatch< E>::create( event);
               }

               friend std::ostream& operator << ( std::ostream& out, const Collection& value)
               {
                  out << "{ ";
                  value.print( out, Events{}...);
                  return out << '}';
               }

            private:

               template< typename M>
               void do_subscription( const M& message) { }


               template< typename M, typename E, typename... Es>
               void do_subscription( const M& message, E&&, Es&&...)
               {
                  event< E>().subscription( message);
                  do_subscription( message, Es{}...);
               }

               void do_remove( strong::process::id pid) { }


               template< typename E, typename... Es>
               void do_remove( strong::process::id pid, E&&, Es&&...)
               {
                  event< E>().remove( pid);
                  do_remove( pid, Es{}...);
               }

               template< typename E>
               void print( std::ostream& out, E&&) const
               {
                  out << event< E>();
               }

               template< typename E, typename... Es>
               void print( std::ostream& out, E&&, Es&&...) const
               {
                  out << event< E>() << ", ";
                  print( out, Es{}...);
               }

            };
         } // dispatch
      } // event
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_DISPATCH_H_
