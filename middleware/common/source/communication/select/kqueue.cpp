//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/platform.h"
#if defined(CASUAL_PLATFORM_BSD)

#include "common/communication/select/kqueue.h"

#include "common/communication/select.h"
#include "common/communication/device.h"

#include "common/signal.h"
#include "common/posix.h"
#include "common/memory.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"

#include "casual/assert.h"

#include <expected>



std::ostream& operator << ( std::ostream& out, const casual::common::communication::select::directive::kevent_t& event);

namespace casual
{
   namespace common::communication::select
   { 
      namespace local
      {
         namespace
         {
            using Filter = directive::detail::Filter;

            // these are mostly to log the kevent_t with user friendly names
            namespace flags
            {
               /*
               #define EV_ADD              0x0001      // add event to kq (implies enable)
               #define EV_DELETE           0x0002      // delete event from kq
               #define EV_ENABLE           0x0004      // enable event
               #define EV_DISABLE          0x0008      // disable event (not reported)
               #define EV_ONESHOT          0x0010      // only report one occurrence 
               #define EV_CLEAR            0x0020      // clear event state after reporting 
               #define EV_RECEIPT          0x0040      // force immediate event output 
               #define EV_DISPATCH         0x0080      // disable event after reporting 
               #define EV_UDATA_SPECIFIC   0x0100      // unique kevent per udata value 
               #define EV_DISPATCH2        (EV_DISPATCH | EV_UDATA_SPECIFIC)
               #define EV_VANISHED         0x0200      // report that source has vanished  
               #define EV_SYSFLAGS         0xF000      // reserved by system 
               #define EV_FLAG0            0x1000      // filter-specific flag 
               #define EV_FLAG1            0x2000      // filter-specific flag 
               #define EV_EOF              0x8000      // EOF detected 
               #define EV_ERROR            0x4000      // error, data contains errno 
               */
               enum struct Flag : uint16_t
               {
                  add = EV_ADD,                    // add event to kq (implies enable)
                  remove = EV_DELETE,              // delete event from kq
                  enable = EV_ENABLE,              // enable event
                  disable = EV_DISABLE,            // disable event (not reported)
                  oneshot = EV_ONESHOT,            // only report one occurrence 
                  clear = EV_CLEAR,                // clear event state after reporting 
                  receipt = EV_RECEIPT,            // force immediate event output 
                  dispatch = EV_DISPATCH,          // disable event after reporting 
                  udata_specific = EV_UDATA_SPECIFIC,    // unique kevent per udata value 
                  dispatch2 = EV_DISPATCH2,        // in combination with EV_DELETE
                  vanished = EV_VANISHED,          // report that source has vanished  
                  sysflags = EV_SYSFLAGS,          // reserved by system 
                  flag0 = EV_FLAG0,                // filter-specific flag 
                  flag1 = EV_FLAG1,                // filter-specific flag 
                  eof = EV_EOF,                    // EOF detected 
                  error = EV_ERROR                 // error, data contains errno 
               };

               constexpr std::string_view description( Flag flag)
               {
                  switch( flag)
                  {
                     case Flag::add: return "add";
                     case Flag::remove: return "remove";
                     case Flag::enable: return "enable";
                     case Flag::disable: return "disable";
                     case Flag::oneshot: return "oneshot";
                     case Flag::clear: return "clear";
                     case Flag::receipt: return "receipt";
                     case Flag::dispatch: return "dispatch";
                     case Flag::udata_specific: return "udata_specific";
                     case Flag::dispatch2: return "dispatch2";
                     case Flag::vanished: return "vanished";
                     case Flag::sysflags: return "sysflags";
                     case Flag::flag0: return "flag0";
                     case Flag::flag1: return "flag1";
                     case Flag::eof: return "eof";
                     case Flag::error: return "error";
                  }
                  return "<unknown>";
               }

               [[maybe_unused]] consteval void casual_enum_as_flag( Flag);

               /*
               NOTE_DELETE    The  unlink() system call was called on
               NOTE_WRITE     A write occurred	on the file referenced
               NOTE_EXTEND    The file	referenced by  the  descriptor
               NOTE_ATTRIB    The  file  referenced by	the descriptor
               NOTE_LINK      The link	count on the file changed.
               NOTE_RENAME    The file	referenced by  the  descriptor
               NOTE_REVOKE    Access  to  the	file  was  revoked via
               NOTE_FUNLOCK   The   file   was	 unlocked  by  calling
               NOTE_LEASE_DOWNGRADE
               NOTE_LEASE_RELEASE A  lease	 break to release the lease is
               NOTE_EXIT    The process has exited.
               NOTE_EXITSTATUS
               NOTE_FORK    The  process  created a child process via
               NOTE_EXEC    The process executed a  new  process  via
               NOTE_SIGNAL  The process was sent a signal. Status can
               NOTE_REAP    The process was reaped by the parent  via
               */
               enum struct FFlags : uint32_t
               {
                  none = 0,                       // no flags
                  delete_ = NOTE_DELETE,          // vnode was removed
                  write = NOTE_WRITE,             // data contents changed
                  extend = NOTE_EXTEND,           // size increased
                  attrib = NOTE_ATTRIB,           // attributes changed
                  link = NOTE_LINK,               // link count changed
                  rename = NOTE_RENAME,           // vnode was renamed
                  revoke = NOTE_REVOKE,           // vnode access was revoked
                  funlock = NOTE_FUNLOCK,         // vnode was unlocked by flock(2)
                  lease_downgrade = NOTE_LEASE_DOWNGRADE, // lease downgrade requested
                  lease_release = NOTE_LEASE_RELEASE,     // lease release requested
                  exit = NOTE_EXIT,               // process exited
                  exit_status = NOTE_EXITSTATUS,  // exited with status
                  fork = NOTE_FORK,               // process forked
                  exec = NOTE_EXEC,               // process exec'd
                  signal = NOTE_SIGNAL,           // shared with EVFILT_SIGNAL
               };

               constexpr std::string_view description( FFlags flag)
               {
                  switch( flag)
                  {
                     case FFlags::none: return "none";
                     case FFlags::delete_: return "delete";
                     case FFlags::write: return "write";
                     case FFlags::extend: return "extend";
                     case FFlags::attrib: return "attrib";
                     case FFlags::link: return "link";
                     case FFlags::rename: return "rename";
                     case FFlags::revoke: return "revoke";
                     case FFlags::funlock: return "funlock";
                     case FFlags::lease_downgrade: return "lease_downgrade";
                     case FFlags::lease_release: return "lease_release";
                     case FFlags::exit: return "exit";
                     case FFlags::exit_status: return "exit_status";
                     case FFlags::fork: return "fork";
                     case FFlags::exec: return "exec";
                     case FFlags::signal: return "signal";
                  }
                  return "<unknown>";
               }

               [[maybe_unused]] consteval void casual_enum_as_flag( FFlags);

            } // flags

            strong::file::descriptor::id kqueue_create()
            {
               Trace trace{ "common::communication::select::kqueue::create"};

               auto descriptor = strong::file::descriptor::id{ ::kqueue()};
               
               if( ! descriptor)
                  code::system::raise( "failed to create kqueue");

               return descriptor;
            }

            constexpr auto create_event( auto identifier, Filter filter, flags::Flag flags, flags::FFlags fflags = flags::FFlags::none)
            {

               // struct kevent {
               //    uintptr_t   ident;        /* identifier for this event */
               //    short	      filter;       /* filter for event */
               //    u_short     flags;	     /* action flags for kqueue */
               //    u_int	      fflags;       /* filter flag value */
               //    int64_t	   data;	        /* filter data value */
               //    void	      *udata;       /* opaque user data identifier */
               //    uint64_t	   ext[4];       /* extensions */
               // };

               using ident_t = decltype( directive::kevent_t::ident);

               return directive::kevent_t{
                  .ident = static_cast< ident_t>( identifier),
                  .filter = std::to_underlying( filter),
                  .flags = std::to_underlying( flags),
                  .fflags = std::to_underlying( fflags),
                  .data = 0
               };
            }
            
            std::expected< int, std::errc> kevent( 
               strong::file::descriptor::id kqueue, 
               std::span< const directive::kevent_t> changelist,
               std::span< directive::kevent_t> eventlist) noexcept
            {
               Trace trace{ "common::communication::select::kqueue::kevent"};

               auto result = ::kevent(
                  kqueue.value(), 
                  changelist.data(), 
                  static_cast< int>( changelist.size()), 
                  eventlist.data(), 
                  static_cast< int>( eventlist.size()), 
                  nullptr // timeout
               );

               if( result < 0)
               {
                  auto error = code::system::last::error();
                  log::debug( error, " ::kevent - kqueue: ", kqueue, " - changelist: ", changelist, " - eventlist: ", eventlist);
                  return std::unexpected{ error};
               }
               
               return result;
            }

            void change_event( strong::file::descriptor::id kqueue, const directive::kevent_t& event)
            {
               Trace trace{ "common::communication::select::kqueue::change_event"};

               auto changelist = std::span{ &event, 1};

               local::kevent( kqueue, changelist, {});
            }

            void change_events( strong::file::descriptor::id kqueue, const auto& events)
            {
               Trace trace{ "common::communication::select::kqueue::change_events"};

               local::kevent( kqueue, events, {});
            }

            auto add_signals( strong::file::descriptor::id kqueue)
            {
               Trace trace{ "common::communication::select::kqueue::add_signals"};

               // not sure if this is all the signals we want events for. Might be some more.
               constexpr auto signals = array::make(
                  create_event( code::signal::child, Filter::signal, flags::Flag::add),
                  create_event( code::signal::alarm, Filter::signal, flags::Flag::add),
                  create_event( code::signal::terminate, Filter::signal, flags::Flag::add),
                  create_event( code::signal::user, Filter::signal, flags::Flag::add));

               change_events( kqueue, signals);
            }

            auto filter_predicate( Filter filter)
            {
               return [ filter]( const directive::kevent_t& event)
               {
                  return Filter{ event.filter} == filter;
               };
            }
            
         } // <unnamed>
      } // local

      namespace directive
      {      
         namespace detail
         {
            std::string_view description( Filter filter)
            {
               switch( filter)
               {
                  case Filter::none: return "none";
                  case Filter::read: return "read";
                  case Filter::write: return "write";
                  case Filter::vnode: return "vnode";
                  case Filter::signal: return "signal";
                  case Filter::timer: return "timer";
                  case Filter::user: return "user";
                  case Filter::proc: return "proc";
               }
               return "<unknown>";
            }
         } // detail

         //! @returns the descriptor from the event
         strong::file::descriptor::id descriptor( const directive::kevent_t& event) noexcept
         {
            return strong::file::descriptor::id{ static_cast< platform::file::descriptor::native::type>( event.ident)};
         }

      } // directive


      Directive::Directive()
         : m_kqueue{ local::kqueue_create()}
      {
         Trace trace{ "common::communication::select::Directive::Directive"};

         // add the signal events
         local::add_signals( m_kqueue);
      }

      Directive::~Directive()
      {
         if( m_kqueue)
            ::close( m_kqueue.value());
      }

      Directive::Directive( Directive&& other) noexcept = default;

      Directive& Directive::operator = ( Directive&& other) noexcept = default;
      

      void Directive::read_add( strong::file::descriptor::id descriptor)
      {
         add( descriptor, directive::detail::Filter::read);
      }

      void Directive::read_remove( strong::file::descriptor::id descriptor)
      {
         remove( descriptor, directive::detail::Filter::read);
      }

      void Directive::write_add( strong::file::descriptor::id descriptor)
      {
        add( descriptor, directive::detail::Filter::write);
      }

      void Directive::write_remove( strong::file::descriptor::id descriptor)
      {
         remove( descriptor, directive::detail::Filter::write);
      }

      void Directive::add( strong::file::descriptor::id descriptor, directive::detail::Filter filter)
      {
         Trace trace{ "common::communication::select::Directive::add"};

         auto entry = directive::detail::Entry{
            .descriptor = descriptor,
            .filter = filter
         };

         if( algorithm::find( m_entries, entry))
            return; // already added

         log::debug( "adding: ", entry);

         auto event = local::create_event( descriptor.value(), filter, local::flags::Flag::add);
         local::change_event( m_kqueue, event);

         m_entries.push_back( entry);
         m_events.resize( m_entries.size());
      }

      void Directive::remove( strong::file::descriptor::id descriptor, directive::detail::Filter filter)
      {
         Trace trace{ "common::communication::select::Directive::remove"};

         auto entry = directive::detail::Entry{
            .descriptor = descriptor,
            .filter = filter
         };

         if( auto found = algorithm::find( m_entries, entry))
         {
            log::debug( "removing: ", *found);

            m_entries.erase( std::begin( found));
            m_events.resize( m_entries.size());

            auto event = local::create_event( descriptor.value(), filter, local::flags::Flag::remove);
            local::change_event( m_kqueue, event);
         }

      }
      

      namespace dispatch::detail
      {

         directive::Ready select( Directive& directive)
         {
            Trace trace{ "common::communication::select::dispatch::detail::select"};
            log::debug( "directive: ", directive);

            // fits in L1
            static_assert( sizeof( select::directive::kevent_t) < 64);

            // we've added the signal events, now we can check pending signals
            // before we call kevent64, which will block until an event is ready
            if( signal::pending())
               code::raise::error( code::casual::interrupted);

            // use the event "buffer"
            auto& events = directive.events();

            auto event_count = local::kevent( 
               directive.descriptor(), 
               {}, 
               events);

            if( ! event_count)
               code::system::raise( event_count.error(), "kevent failed");


            auto ready = range::make( events.data(), *event_count);
            log::debug( "ready: ", ready);
            
            auto [ read, rest1] = algorithm::partition( ready, local::filter_predicate( directive::detail::Filter::read));
            auto [ write, rest2] = algorithm::partition( rest1, local::filter_predicate( directive::detail::Filter::write));

            log::debug( "read: ", read);
            log::debug( "write: ", write);
            log::debug( "rest: ", rest2);

            // we need to check signal events in 'rest'.
            if( algorithm::find_if( rest2, local::filter_predicate( directive::detail::Filter::signal)))
               code::raise::error( code::casual::interrupted);

            return directive::Ready{
               .read = read,
               .write = write
            };

         }

      } // dispatch::detail

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

std::ostream& operator << ( std::ostream& out, const casual::common::communication::select::directive::kevent_t& event)
{
   using namespace casual::common::communication::select::local;

   return casual::common::stream::write( out, "{ ident: ", event.ident, 
      ", filter: ", Filter{ event.filter}, 
      ", flags: ", flags::Flag{ event.flags}, 
      ", fflags: ", flags::FFlags{ event.fflags}, 
      ", data: ", event.data, 
      ", udata: ", event.udata,
      "}");
}


#endif // CASUAL_PLATFORM_BSD

