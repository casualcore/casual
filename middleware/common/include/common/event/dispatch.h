//!
//! casual 
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
            void remove( platform::pid::type pid);

         protected:

            common::message::pending::Message pending( common::communication::message::Complete&& complete) const;

            std::vector< common::process::Handle> subscribers;

            bool exists( platform::ipc::id::type queue) const;
         };

         template< typename Event>
         struct Dispatch : base_dispatch
         {

            using message_type = common::message::Type;

            void subscription( const message::event::subscription::Begin& message)
            {
               if( ( message.types.empty() || range::find( message.types, Event::type())) && ! exists( message.process.queue))
               {
                  subscribers.push_back( message.process);
               }
            }

            void subscription( const message::event::subscription::End& message)
            {
               range::trim( subscribers, range::remove_if( subscribers, [&]( auto& v){
                  return v.queue == message.process.queue;
               }));
            }

            void remove( platform::pid::type pid)
            {
               range::trim( subscribers, range::remove_if( subscribers, [pid]( auto& v){
                  return pid == v.pid;
               }));
            }

            void subscription( platform::ipc::id::type queue)
            {
               if( ! exists( queue))
               {
                  subscribers.emplace_back( 0, queue);
               }
            }

            explicit operator bool () const { return ! subscribers.empty();}

            common::message::pending::Message operator () ( const Event& event) const
            {
               return pending( marshal::complete( event));
            }

            friend std::ostream& operator << ( std::ostream& out, const Dispatch& value)
            {
               return out << "{ event: " << Event::type()
                     << ", subscribers: " << range::make( value.subscribers)
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

               void remove( platform::pid::type pid)
               {
                  do_remove( pid, Events{}...);
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

               void do_remove( platform::pid::type pid) { }


               template< typename E, typename... Es>
               void do_remove( platform::pid::type pid, E&&, Es&&...)
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
