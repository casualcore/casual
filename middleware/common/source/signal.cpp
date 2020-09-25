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
   namespace common
   {
      namespace signal
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
                  std::atomic< long> global_total_pending{ 0};

                  template< code::signal signal>
                  struct basic_pending
                  {
                     static std::atomic< bool> pending;
                  };
                  template< code::signal Signal>
                  std::atomic< bool> basic_pending< Signal>::pending{ false};

                  template< code::signal signal>
                  void clear() 
                  {
                     basic_pending< signal>::pending = false;
                  }

                  template< code::signal signal, code::signal next, code::signal... signals>
                  void clear()
                  {
                     clear< signal>();
                     clear< next, signals...>();
                  }

                  template< code::signal signal>
                  bool pending( signal::Set mask)
                  {
                     return basic_pending< signal>::pending.load() && ! mask.exists( signal);
                  }


                  template< code::signal signal, code::signal next, code::signal... signals>
                  bool pending( signal::Set mask)
                  {
                     return pending< signal>( mask) || 
                        pending< next, signals...>( mask);
                  }

                  template< code::signal Signal>
                  void signal_callback( platform::signal::native::type signal)
                  {
                     if( ! basic_pending< Signal>::pending.exchange( true))
                        ++global_total_pending;
                  }

                  template< typename H>
                  void registration( code::signal signal, H&& handler, int flags = 0)
                  {
                     struct sigaction sa = {};

                     sa.sa_handler = handler;
                     sa.sa_flags = flags;

                     if( ::sigaction( cast::underlying( signal), &sa, nullptr) == -1)
                     {
                        std::cerr << "failed to register handle for signal: " << signal << " - "  << code::system::last::error() << '\n';
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
                        if( --handler::global_total_pending >= 0)
                        {
                           // if no signal was consumed based on the mask, we need to restore the global
                           if( ! dispatch( mask))
                              ++handler::global_total_pending;
                        }
                        else
                        {
                           // There was no pending signals, and we need to 'restore' the global
                           ++handler::global_total_pending;
                        }
                     }

                     bool pending( signal::Set set)
                     {
                        return handler::global_total_pending.load() > 0 
                           && handler::pending<                            
                              code::signal::child,
                              code::signal::user,
                              code::signal::hangup,
                              code::signal::alarm,
                              code::signal::terminate,
                              code::signal::quit,
                              code::signal::interrupt>( set);
                     }

                     void registration( code::signal signal, common::function< void()> callback)
                     {
                       if( auto found = algorithm::find_if( m_handlers, [signal]( auto& handler){ return handler.signal == signal;}))
                           found->callbacks.push_back( std::move( callback));
                        else 
                           code::raise::generic( code::casual::invalid_argument, log::stream::get( "error"), "failed to find signal handler for: ", signal);
                     }

                     callback::detail::Replace replace( callback::detail::Replace wanted)
                     {
                        if( auto found = algorithm::find_if( m_handlers, [signal = wanted.signal]( auto& handler){ return handler.signal == signal;}))
                        {
                           std::swap( wanted.callbacks, found->callbacks);
                        }
                        return wanted;
                     }

                     void clear()
                     {
                        global_total_pending = 0;

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
                        using callbacks_type = std::vector< common::function< void()>>;

                        bool operator () ( const signal::Set& current)
                        {
                           return disptacher( current, callbacks);
                        }

                        code::signal signal{};
                        common::function< bool( const signal::Set&, callbacks_type&)> disptacher;
                        callbacks_type callbacks;
                        
                     };

                     template< code::signal signal>
                     static auto create_dispatcher()
                     {
                        return []( const signal::Set& current, Handler::callbacks_type& callbacks)
                        {
                           // check that: not masked and the signal was pending
                           if( ! current.exists( signal) && basic_pending< signal>::pending.exchange( false))
                           {
                              // Signal is not blocked
                              log::line( log::debug, "signal: handling signal: ", signal);

                              // if we don't have any handlers we need to propagate the signal via exception.
                              if( callbacks.empty())
                                 code::raise::log( signal, "raise signal");
                              
                              // execute the "callbacks"
                              algorithm::for_each( callbacks, []( auto& callback){ callback();});

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
                        result.disptacher = create_dispatcher< signal>();
                        return result;
                     }

                     template< code::signal signal, typename C>
                     static Handler create_handler( C&& callback, int flags = 0)
                     {
                        auto result = create_handler< signal>( flags);
                        result.callbacks.push_back( std::move( callback));
                        return result;
                     }

                     std::vector< Handler> m_handlers = {

                        Handle::create_handler< code::signal::child>( SA_NOCLDSTOP),
                        Handle::create_handler< code::signal::alarm>(),
                        Handle::create_handler< code::signal::user>(),

                        // reopen 'casual.log' on hangup
                        Handle::create_handler< code::signal::hangup>( []()
                        {
                           log::stream::reopen();
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
               return local::handler::global_total_pending.load();
            }
         } // current

         namespace callback
         {
            namespace detail
            {
               void registration( code::signal signal, common::function< void()> callback)
               {
                  local::handler::Handle::instance().registration( signal, std::move( callback));
               }

               Replace replace( Replace wanted)
               {
                  return local::global_handler.replace( std::move( wanted));
               }
               
            } // detail
         } // callback

         namespace timer
         {
            namespace local
            {
               namespace
               {

                  platform::time::unit convert( const itimerval& value)
                  {
                     if( value.it_value.tv_sec == 0 && value.it_value.tv_usec == 0)
                        return platform::time::unit::min();

                     return std::chrono::seconds( value.it_value.tv_sec) + std::chrono::microseconds( value.it_value.tv_usec);
                  }

                  platform::time::unit get()
                  {
                     itimerval old;

                     if( ::getitimer( ITIMER_REAL, &old) != 0)
                        code::system::raise( "timer::get");

                     return convert( old);
                  }

                  platform::time::unit set( itimerval& value)
                  {
                     itimerval old;

                     if( ::setitimer( ITIMER_REAL, &value, &old) != 0)
                        code::system::raise( "timer::set");

                     log::line( verbose::log, "timer set: ",
                           value.it_value.tv_sec, ".", std::setw( 6), std::setfill( '0'), value.it_value.tv_usec, "s - was: ",
                           old.it_value.tv_sec, "." ,std::setw( 6), std::setfill( '0'), old.it_value.tv_usec, "s");

                     return convert( old);
                  }
               } // <unnamed>
            } // local

            platform::time::unit set( platform::time::unit offset)
            {
               auto microseconds = std::chrono::duration_cast< std::chrono::microseconds>( offset);

               if( microseconds <= std::chrono::microseconds::zero())
               {
                  if( offset == platform::time::unit::min())
                  {
                     // Special case == 'unset'
                     return unset();
                  }

                  // We send the signal directly
                  log::line( log::debug, "timer - offset is less than zero: ", offset, " - send alarm directly");
                  signal::send( process::id(), code::signal::alarm);
                  return local::get();
               }
               else
               {
                  
                  itimerval value;
                  value.it_interval.tv_sec = 0;
                  value.it_interval.tv_usec = 0;
                  value.it_value.tv_sec = std::chrono::duration_cast< std::chrono::seconds>( microseconds).count();
                  value.it_value.tv_usec = ( microseconds % std::chrono::seconds{ 1}).count();

                  return local::set( value);
               }
            }

            platform::time::unit get()
            {
               return local::get();
            }

            platform::time::unit unset()
            {
               itimerval value = {};

               return local::set( value);
            }



            Scoped::Scoped( platform::time::unit timeout, const platform::time::point::type& now)
            {
               auto old = timer::set( timeout);

               if( old != platform::time::unit::min())
               {
                  m_old = now + old;
                  log::line( verbose::log, "old timepoint: ", m_old.time_since_epoch());
               }
            }

            Scoped::Scoped( platform::time::unit timeout)
               : Scoped( timeout, platform::time::clock::type::now())
            {
            }

            Scoped::~Scoped()
            {
               if( m_active)
               {
                  if( m_old == platform::time::point::limit::zero())
                     timer::unset();
                  else
                     timer::set( m_old - platform::time::clock::type::now());
               }
            }


            Deadline::Deadline( const platform::time::point::type& deadline, const platform::time::point::type& now)
            {
               if( deadline != platform::time::point::type::max())
                  timer::set( deadline - now);
               else
                  timer::unset();
            }

            Deadline::Deadline( const platform::time::point::type& deadline)
             : Deadline( deadline, platform::time::clock::type::now()) {}


            Deadline::Deadline( platform::time::unit timeout, const platform::time::point::type& now)
             : Deadline( now + timeout, now) {}

            Deadline::Deadline( platform::time::unit timeout)
             : Deadline( timeout, platform::time::clock::type::now()) {}


            Deadline::~Deadline()
            {
               if( m_active)
                  timer::unset();
            }

            Deadline::Deadline( Deadline&&) = default;
            Deadline& Deadline::operator = ( Deadline&&) = default;

         } // timer


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
      } // signal
   } // common
} // casual



