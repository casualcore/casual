//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/signal.h"
#include "casual/platform.h"
#include "common/log.h"
#include "common/flag.h"
#include "common/process.h"
#include "common/chronology.h"
#include "common/memory.h"
#include "common/cast.h"

#include "common/code/raise.h"
#include "common/code/signal.h"
#include "common/code/system.h"
#include "common/result.h"
#include "common/stream.h"



#include <csignal>
#include <cstring>


#include <stack>
#include <thread>
#include <iomanip>
#include <iostream>

#include <sys/time.h>


namespace casual
{
   namespace common::signal
   {
      namespace local
      {
         namespace
         {
            bool send( strong::process::id pid, code::signal signal)
            {
               log::line( verbose::log, "local::signal::send ", signal, " -> pid: ", pid);

               return posix::log::result( 
                  ::kill( pid.value(), cast::underlying( signal)), 
                  "failed to send signal - ", signal, " -> pid: ", pid);
            }

            namespace handler
            {
               namespace global::total
               {
                  std::atomic< platform::size::type> pending{ 0};   
               } // global::total
               

               template< code::signal signal>
               struct basic_pending
               {
                  inline static std::atomic< bool> pending{ false};
                  inline static void handle() {}; // no-op default 
               };

               template<>
               struct basic_pending< code::signal::hangup>
               {
                  inline static std::atomic< bool> pending{ false};

                  inline static void handle()
                  {
                     // set state so the log will be reopen on the next write
                     log::stream::reopen();
                  }
               };

               template< code::signal... signals>
               void clear()
               {
                  ( ( basic_pending< signals>::pending = false) , ...);
               }

               namespace detail
               {
                  //! TODO: remove and move this inline in the fold-expression when we don't need to support g++ 9.3.1
                  template< code::signal signal>
                  bool pending( signal::Set mask)
                  {
                     return basic_pending< signal>::pending.load() && ! mask.exists( signal);
                  }
               } // detail

               template< code::signal... signals>
               bool pending( signal::Set mask)
               {
                  return ( detail::pending< signals>( mask) || ...);
               }

               template< code::signal Signal>
               void signal_callback( platform::signal::native::type signal)
               {
                  // might do stuff, in a signal safe way...
                  basic_pending< Signal>::handle();

                  if( ! basic_pending< Signal>::pending.exchange( true))
                     ++global::total::pending;
               }

               template< typename H>
               void registration( code::signal signal, H&& handler, int flags = 0)
               {
                  struct sigaction sa = {};

                  sa.sa_handler = handler;
                  sa.sa_flags = flags;

                  if( ::sigaction( cast::underlying( signal), &sa, nullptr) == -1)
                  {
                     stream::write( std::cerr, "failed to register handle for signal: ", signal, " - " , code::system::last::error(), '\n');
                     code::system::raise( "failed to register handle for signal");
                  }
               }

               struct Handle
               {
                  static Handle& instance()
                  {
                     static Handle handle;
                     return handle;
                  }

                  void handle( signal::Set mask)
                  {
                     // We only allow one thread at a time to actually handle the
                     // pending signals
                     if( --handler::global::total::pending >= 0)
                     {
                        // if no signal was consumed based on the mask, we need to restore the global
                        if( ! dispatch( mask))
                           ++handler::global::total::pending;
                     }
                     else
                     {
                        // There was no pending signals, and we need to 'restore' the global
                        ++handler::global::total::pending;
                     }
                  }

                  bool pending( signal::Set set)
                  {
                     return handler::global::total::pending.load() > 0 
                        && handler::pending<                            
                           code::signal::child,
                           code::signal::user,
                           code::signal::hangup,
                           code::signal::alarm,
                           code::signal::terminate,
                           code::signal::quit,
                           code::signal::interrupt>( set);
                  }

                  void registration( code::signal signal, callback::callback_type callback)
                  {
                     if( auto found = algorithm::find( m_handlers, signal))
                        found->callback = std::move( callback);
                     else 
                        code::raise::error( code::casual::invalid_argument, "failed to find signal handler for: ", signal);
                  }

                  callback::detail::Replace replace( callback::detail::Replace wanted)
                  {
                     if( auto found = algorithm::find( m_handlers, wanted.signal))
                        std::swap( wanted.callback, found->callback);

                     return wanted;
                  }

                  void clear()
                  {
                     global::total::pending = 0;

                     local::handler::clear< 
                        code::signal::child,
                        code::signal::user,
                        code::signal::hangup,
                        code::signal::alarm,
                        code::signal::terminate,
                        code::signal::quit,
                        code::signal::interrupt>();
                  }

               private:

                  Handle() 
                  {
                     // make sure we ignore sigpipe
                     local::handler::registration( code::signal::pipe, SIG_IGN);
                  }

                  bool dispatch( signal::Set current)
                  {
                     return algorithm::any_of( m_handlers, [&current]( auto& handler){ return handler( current);});
                  }

                  struct Handler
                  {
                     bool operator () ( const signal::Set& current)
                     {
                        return dispatcher( current, callback);
                     }

                     friend bool operator == ( const Handler& lhs, code::signal rhs) { return lhs.signal == rhs;}

                     code::signal signal{};
                     common::function< bool( const signal::Set&, callback::callback_type&)> dispatcher;
                     callback::callback_type callback;
                     
                  };

                  template< code::signal signal>
                  static auto create_dispatcher()
                  {
                     return []( const signal::Set& current, auto& callback)
                     {
                        // check that: not masked and the signal was pending
                        if( ! current.exists( signal) && basic_pending< signal>::pending.exchange( false))
                        {
                           // Signal is not blocked
                           log::line( log::debug, "signal: handling signal: ", signal);

                           // if we don't have any handler we need to propagate the signal via exception.
                           if( ! callback)
                              code::raise::error( signal, "raise signal");
                           
                           // execute the "callback"
                           callback();

                           return true;
                        }
                        return false;
                     };
                  }

                  template< code::signal signal>
                  static Handler create_handler( int flags = 0)
                  {
                     // Register the signal handler for this signal
                     local::handler::registration( signal, &signal_callback< signal>, flags);

                     Handler result;
                     result.signal = signal;
                     result.dispatcher = create_dispatcher< signal>();
                     return result;
                  }

                  template< code::signal signal, typename C>
                  static Handler create_handler( C&& callback, int flags = 0)
                  {
                     auto result = create_handler< signal>( flags);
                     result.callback = std::forward< C>( callback);
                     return result;
                  }

                  std::vector< Handler> m_handlers = {

                     Handle::create_handler< code::signal::child>( SA_NOCLDSTOP),
                     Handle::create_handler< code::signal::alarm>(),
                     Handle::create_handler< code::signal::user>(),

                     Handle::create_handler< code::signal::hangup>( []()
                     {
                        // no-op. the signal handler takes care of setting state
                        // for the log so it will be reopened on the next write.
                     }),

                     Handle::create_handler< code::signal::terminate>(),
                     Handle::create_handler< code::signal::quit>(),
                     Handle::create_handler< code::signal::interrupt>(),
                  };
               };

            } // handler

            // We need to instantiate the handler globally to trigger signal-handler-registration
            handler::Handle& global_handler = handler::Handle::instance();
         } // <unnamed>
      } // local



      void dispatch()
      {
         dispatch( signal::mask::current());
      }

      void dispatch( signal::Set mask)
      {
         local::global_handler.handle( mask);
      }

      bool pending( signal::Set mask)
      {
         return local::global_handler.pending( mask);
      }

      void clear()
      {
         local::global_handler.clear();
      }

      namespace current
      {
         long pending()
         {
            return local::handler::global::total::pending.load();
         }
      } // current

      namespace callback
      {
         namespace detail
         {
            void registration( code::signal signal, callback_type callback)
            {
               local::handler::Handle::instance().registration( signal, std::move( callback));
            }

            Replace replace( Replace wanted)
            {
               return local::global_handler.replace( std::move( wanted));
            }
            
         } // detail
      } // callback

      bool send( strong::process::id pid, code::signal signal)
      {
         return local::send( pid, signal);
      }


      Set::Set() : Set( empty_t{}) {}

      Set::Set( set::type set) : set( std::move( set)) {}


      Set::Set( std::initializer_list< code::signal> signals) : Set( empty_t{})
      {
         for( auto&& signal : signals)
            add( signal);
      }

      void Set::add( code::signal signal)
      {
         sigaddset( &set, cast::underlying( signal));
      }

      void Set::remove( code::signal signal)
      {
         sigdelset( &set, cast::underlying( signal));
      }


      bool Set::exists( code::signal signal) const
      {
         return sigismember( &set, cast::underlying( signal)) == 1;
      }



      Set::Set( filled_t)
      {
         sigfillset( &set);
      }
      Set::Set( empty_t)
      {
         sigemptyset( &set);
      }
      

      std::ostream& operator << ( std::ostream& out, const Set& value)
      {
         out << "[";

         constexpr code::signal signals[] = { code::signal::alarm, code::signal::child, code::signal::interrupt, code::signal::kill, code::signal::pipe, code::signal::quit, code::signal::terminate, code::signal::user};

         bool first = true;
         for( auto& signal : signals)
         {
            if( value.exists( signal))
            {
               if( ! first)
                  common::stream::write( out, ", ", signal);
               else
               {
                  common::stream::write( out, signal);
                  first = false;
               }
            }
         }

         return out << ']';
      }

      namespace set
      {
         signal::Set filled()
         {
            return { signal::Set::filled_t{}};
         }

         signal::Set empty()
         {
            return {};
         }
      } // set

      namespace mask
      {
         signal::Set set( signal::Set mask)
         {
            const auto set{mask.set};
            pthread_sigmask( SIG_SETMASK, &set, &mask.set);
            return mask;
         }

         signal::Set block( signal::Set mask)
         {
            const auto set{mask.set};
            pthread_sigmask( SIG_BLOCK, &set, &mask.set);
            return mask;
         }

         signal::Set unblock( signal::Set mask)
         {
            const auto set{mask.set};
            pthread_sigmask( SIG_UNBLOCK, &set, &mask.set);
            return mask;
         }

         signal::Set block()
         {
            return set( set::filled());
         }


         signal::Set current()
         {
            signal::Set mask;
            pthread_sigmask( SIG_SETMASK, nullptr, &mask.set);
            return mask;
         }

      } // mask

      namespace thread
      {
         void send( std::thread& thread, code::signal signal)
         {
            log::line( log::debug, "signal::thread::send thread: ", thread.get_id(), " signal: ", signal);

            send( thread.native_handle(), signal);
         }

         void send( common::thread::native::type thread, code::signal signal)
         {
            if( pthread_kill( thread, 0) == 0)
            {
               if( pthread_kill( thread, cast::underlying( signal)) != 0)
                     log::line( log::category::error, "failed to send signal - ", signal, " -> thread: ", thread, " - error: " , code::system::last::error());
            }
         }

         void send( code::signal signal)
         {
            log::line( log::debug, "signal::thread::send current thread - signal: ", signal);
            send( common::thread::native::current(), signal);
         }


         namespace scope
         {
            Reset::Reset( signal::Set mask) : m_mask( std::move( mask)) {}

            Reset::~Reset()
            {
               if( m_active)
                  mask::set( m_mask);
            }

            const signal::Set& Reset::previous() const
            {
               return m_mask;
            }



            Mask::Mask( signal::Set mask) : Reset( mask::set( mask)) {}

            Block::Block() : Reset( mask::block()) {}
            Block::Block( signal::Set mask) : Reset( mask::block( mask)) {}
            Unblock::Unblock( signal::Set mask) : Reset( mask::unblock( mask)) {}

         } // scope

      } // thread

   } // common::signal
} // casual



