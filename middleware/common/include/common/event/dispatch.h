//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/event.h"

#include "common/communication/message.h"
#include "common/serialize/native/complete.h"

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

            template< typename ID>
            bool exists( ID id) const
            {
               return ! algorithm::find_if( m_subscribers, [id]( auto& process){
                  return process == id;
               }).empty();
            }
         };

         template< typename Event>
         struct Dispatch : base_dispatch
         {

            using message_type = common::message::Type;

            void subscription( const message::event::subscription::Begin& message)
            {
               if( ( message.types.empty() || algorithm::find( message.types, Event::type())) && ! exists( message.process.ipc))
               {
                  m_subscribers.push_back( message.process);
               }
            }

            void subscription( const message::event::subscription::End& message)
            {
               algorithm::trim( m_subscribers, algorithm::remove_if( m_subscribers, [&]( auto& v){
                  return v.ipc == message.process.ipc;
               }));
            }

            //! remove subscribers with pid or ipc
            template< typename ID>
            void remove( ID id)
            {
               algorithm::trim( m_subscribers, algorithm::remove_if( m_subscribers, [id]( auto& process){
                  return process == id;
               }));
            }

            void subscription( strong::ipc::id ipc)
            {
               if( ! exists( ipc))
                  m_subscribers.emplace_back( 0, ipc);
            }

            explicit operator bool () const { return ! m_subscribers.empty();}

            common::message::pending::Message create( const Event& event) const
            {
               return pending( serialize::native::complete( event));
            }

            // for logging only
            CASUAL_LOG_SERIALIZE(
            {
               CASUAL_SERIALIZE_NAME( Event::type(), "event");
               CASUAL_SERIALIZE_NAME( m_subscribers, "subscribers");
            })
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
      } // event
   } // common
} // casual


