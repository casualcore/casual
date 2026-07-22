//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/platform.h"
#if defined(CASUAL_PLATFORM_LINUX)

#include "common/communication/select/epoll.h"


#include "common/communication/select.h"
#include "common/communication/device.h"

#include "common/signal.h"
#include "common/posix.h"
#include "common/memory.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include "casual/assert.h"

//#include <poll.h>

#include <expected>



namespace casual
{
   using namespace common;

   namespace common::communication::select
   {
      namespace local
      {
         namespace
         {
            using Flags = directive::detail::Flags;

            enum struct update_op : int 
            {
               add = EPOLL_CTL_ADD,
               modify = EPOLL_CTL_MOD,
               remove = EPOLL_CTL_DEL
            };

            constexpr std::string_view description( update_op op)
            {
               switch( op)
               {
                  case update_op::add: return "add";
                  case update_op::modify: return "modify";
                  case update_op::remove: return "remove";
               }
               return "unknown";
            }
            

            constexpr std::string_view description( Flags flag)
            {
               switch( flag)
               {
                  case Flags::none: return "none";
                  case Flags::in: return "in";
                  case Flags::out: return "out";
                  case Flags::rd_hup: return "rd_hup";
                  case Flags::prio: return "prio";
                  case Flags::error: return "error";
                  case Flags::hup: return "hup";
                  case Flags::edge: return "edge";
                  case Flags::oneshot: return "oneshot";
                  case Flags::wakeup: return "wakeup";
                  case Flags::exclusive: return "exclusive";
               }
               return "<unknown>";
            }

            auto crate_event( strong::file::descriptor::id descriptor, Flags flags)
            {
               return ::epoll_event{
                  .events = std::to_underlying( flags),
                  .data = { .fd = descriptor.value()}
               };
            }

            strong::file::descriptor::id epoll_create()
            {
               Trace trace{ "common::communication::select::local::epoll_create"};
               auto descriptor = strong::file::descriptor::id{ ::epoll_create1( EPOLL_CLOEXEC)};
               
               if( ! descriptor)
                  code::system::raise( "failed to create epoll");

               return descriptor;
            }

            auto update( strong::file::descriptor::id epoll, update_op op, strong::file::descriptor::id descriptor, ::epoll_event* event = nullptr)
            {
               Trace trace{ "common::communication::select::local::update"};
               log::debug( "epoll: ", epoll, ", op: ", description( op), ", descriptor: ", descriptor, ", event: ", event ? *event : ::epoll_event{});

               return posix::expected( ::epoll_ctl( epoll.value(), std::to_underlying( op), descriptor.value(), event));
            }

            std::expected< int, std::errc> epoll_wait( strong::file::descriptor::id epoll, std::span< ::epoll_event> events)
            {
               Trace trace{ "common::communication::select::local::epoll_wait"};

               // block all signals
               signal::scope::Block block;

               // check if we've got signals before the block.
               if( signal::pending( block.previous()))
                  return std::unexpected{ std::errc::interrupted};

               // int epoll_pwait(int epfd, struct epoll_event events[.maxevents],
               //       int maxevents, int timeout,
               //       const sigset_t *_Nullable sigmask);

               constexpr int indefinitely = -1;

               return posix::expected( ::epoll_pwait( 
                  epoll.value(), 
                  events.data(), 
                  events.size(), 
                  indefinitely, // timeout
                  &block.previous().set));
            }

         } // <unnamed>
      } // local

      namespace directive
      {
         namespace detail
         {
            std::string_view description( Flags flag)
            {
               switch( flag)
               {
                  case Flags::none: return "none";
                  case Flags::in: return "in";
                  case Flags::out: return "out";
                  case Flags::rd_hup: return "rd_hup";
                  case Flags::prio: return "prio";
                  case Flags::error: return "error";
                  case Flags::hup: return "hup";
                  case Flags::edge: return "edge";
                  case Flags::oneshot: return "oneshot";
                  case Flags::wakeup: return "wakeup";
                  case Flags::exclusive: return "exclusive";
               }
               return "<unknown>";
            }
         } // detail

         strong::file::descriptor::id descriptor( const ::epoll_event& event) noexcept
         {
            return strong::file::descriptor::id{ event.data.fd};
         }
         
      } // directive
      
      
      Directive::Directive()
         : m_epoll{ local::epoll_create()}
      {}
         
      Directive::~Directive()
      {
         if( m_epoll)
            ::close( m_epoll.value());
      }

      Directive::Directive( Directive&& other) noexcept
         : m_epoll{ std::exchange( other.m_epoll, {})},
           m_entries{ std::move( other.m_entries)},
           m_events{ std::move( other.m_events)}
      {}
      
      Directive& Directive::operator = ( Directive&& other) noexcept
      {
         m_epoll = std::exchange( other.m_epoll, {});
         m_entries = std::move( other.m_entries);
         m_events = std::move( other.m_events);
         return *this;
      }
      

      void Directive::read_add( strong::file::descriptor::id descriptor)
      {
         add( descriptor, directive::detail::Flags::in);
      }

      void Directive::read_remove( strong::file::descriptor::id descriptor)
      {
         remove( descriptor, directive::detail::Flags::in);
      }

      void Directive::write_add( strong::file::descriptor::id descriptor)
      {
         add( descriptor, directive::detail::Flags::out);
      }

      void Directive::write_remove( strong::file::descriptor::id descriptor)
      {
         remove( descriptor, directive::detail::Flags::out);
      }
      
      void Directive::add( strong::file::descriptor::id descriptor, directive::detail::Flags flag)
      {
         Trace trace{ "common::communication::select::Directive::add"};
         log::debug( "descriptor: ", descriptor, ", flag: ", flag);

         if( auto found = algorithm::find( m_entries, descriptor))
         {
            // we've got en entry, lets se if we need to update it.
            if( flag::contains( found->event, flag))
               return; // we already have this flag set.

            found->event |= flag; // update the event

            // update the epoll
            auto event = local::crate_event( descriptor, found->event);
            local::update( m_epoll, local::update_op::modify, descriptor, &event);
         
         }
         else
         {
            // not added, add it
            auto event = local::crate_event( descriptor, flag);
            local::update( m_epoll, local::update_op::add, descriptor, &event);

            m_entries.push_back( directive::detail::Entry{ .descriptor = descriptor, .event = flag});
            m_events.resize( m_entries.size());
         }
      }

      void Directive::remove( strong::file::descriptor::id descriptor, directive::detail::Flags flag)
      {
         Trace trace{ "common::communication::select::Directive::remove"};
         log::debug( "descriptor: ", descriptor, ", flag: ", flag);

         if( auto found = algorithm::find( m_entries, descriptor))
         {
            found->event -= flag; // remove the flag

            if( ! flag::empty( found->event))
            {
               // update the epoll
               auto event = local::crate_event( descriptor, found->event);
               local::update( m_epoll, local::update_op::modify, descriptor, &event);
            }
            else
            {
               // if we have no flags left, remove the entry
               local::update( m_epoll, local::update_op::remove, descriptor);
               m_entries.erase( std::begin( found));
               m_events.resize( m_entries.size());
            }
         }
      }



      namespace dispatch
      {
         namespace detail
         {

            directive::Ready ready( std::span< const directive::detail::Entry> entries, std::span< ::epoll_event> events)
            {
               using Flags = directive::detail::Flags;

               auto deduce_flags = [ entries]( const ::epoll_event& event)
               {
                  auto flags = Flags{ event.events};

                  if( ( flags & ( Flags::in | Flags::out)) == Flags::none)
                  {
                     // we got some "error" events, we need to check what the corrsponding entry has, 
                     // if it's a read or write event.
                     if( auto found = algorithm::find( entries, strong::file::descriptor::id{ event.data.fd}))
                     {
                        if( flag::contains( found->event, Flags::in))
                           return Flags::in; // read
                        else
                           return Flags::out; // write
                     }
                     else
                     {
                        log::error( code::casual::internal_unexpected_value, "no entry found for event: ", event);
                        return Flags::in; // read, we need to put it somewhere, but we don't know where, so we put it in the read section
                     }
                  }

                  return flags;
               };

               
               auto has_read = [ deduce_flags]( const ::epoll_event& event)
               {
                  return flag::contains( deduce_flags( event), Flags::in);
               };

               auto has_not_write = [ deduce_flags]( const ::epoll_event& event)
               {
                  return ! flag::contains( deduce_flags( event), Flags::out);
               };

               // Events can be both read and write, so we need to partition them and overlapp the 
               // read/write events. 
               //   read:  [read, read/write]
               //   write: [read/write, write]

               auto read_end = std::ranges::partition( events, has_read);

               auto write_start = std::ranges::partition( std::begin( events), std::begin( read_end), has_not_write);

               auto read = range::make( std::begin( events), std::begin( read_end));
               auto write = range::make( std::begin( write_start), std::end( events));
               
               log::debug( "read: ", read);
               log::debug( "write: ", write);

               return directive::Ready{
                  .read = read,
                  .write = write
               };
            }
     
            directive::Ready select( Directive& directive)
            {
               Trace trace{ "common::communication::select::dispatch::detail::select"};
               log::debug( "directive: ", directive);

               // use the event "buffer"
               auto& events = directive.events();

               auto event_count = local::epoll_wait( 
                  directive.descriptor(),
                  events);

               if( ! event_count)
                  code::system::raise( event_count.error(), "epoll_wait failed");

               auto ready = range::make( events.data(), *event_count);

               return detail::ready( directive.entries(), ready);

            }
         } // detail

      } // dispatch

      namespace local
      {
         namespace
         {
            /*
            // It might be a better idea to use `ppoll` instead of epoll on blocking operations
            // for a single descriptor.
            auto poll( strong::file::descriptor::id descriptor, short flag)
            {
               Trace trace{ "common::communication::select::local::poll"};

               auto event = struct ::pollfd{
                  .fd = descriptor.value(),
                  .events = flag
               }

               // block all signals
               signal::scope::Block block;

               // check if we've got signals before the block.
               signal::dispatch( block.previous());

               posix::result( ::ppoll( &pfd, 1, nullptr, signals.set));
            }
               */
 

            
         } // <unnamed>
      } // local

      namespace block
      {
         void read( strong::file::descriptor::id descriptor)
         {
            Directive directive;
            directive.read_add( descriptor);

            dispatch::detail::select( directive);
         }

         void write( strong::file::descriptor::id descriptor)
         {
            Directive directive;
            directive.write_add( descriptor);

            dispatch::detail::select( directive);
         }
      } // block


   } // common::communication::select
} // casual


std::ostream& operator << ( std::ostream& out, const ::epoll_event& event)
{
   using namespace casual::common::communication::select::local;

   return casual::common::stream::write( out, "{ events: ", Flags{ event.events}, 
      ", data.fd: ", event.data.fd, 
      "}");
}

#endif // CASUAL_PLATFORM_LINUX
