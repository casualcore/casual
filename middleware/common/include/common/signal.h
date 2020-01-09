//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once





#include "casual/platform.h"
#include "common/strong/id.h"
#include "common/code/signal.h"
#include "common/move.h"
#include "common/algorithm.h"
#include "common/thread.h"


#include <atomic>
#include <thread>
#include <cstddef>

// signal
#include <signal.h>

namespace casual
{

   namespace common
   {
      namespace signal
      {
         namespace current
         {
            //! @returns the current number of pending signals
            //!
            //! @note only (?) for unittest
            platform::size::type pending();
         } // current

         namespace timer
         {
            //! Sets a timeout.
            //!
            //! @param offset when the timer kicks in.
            //! @returns previous timeout.
            //!
            //! @note zero and negative offset will trigger a signal directly
            //! @note platform::time::unit::min() has special meaning and will not set any
            //! timeout and will unset current timeout, if any.
            platform::time::unit set( platform::time::unit offset);

            template< typename R, typename P>
            platform::time::unit set( std::chrono::duration< R, P> offset)
            {
               return set( std::chrono::duration_cast< platform::time::unit>( offset));
            }

            //! @return current timeout, platform::time::unit::min() if there isn't one
            platform::time::unit get();

            //! Unset current timeout, if any.
            //!
            //! @return previous timeout, platform::time::unit::min() if there wasn't one
            platform::time::unit unset();

            //! Sets a scoped timout.
            //! dtor will 'reset' previous timeout, if any. Hence enable nested timeouts.
            //!
            //! @note platform::time::unit::min() has special meaning and will not set any
            //! timeout and will unset current timeout, if any (that will be reset by dtor)
            class Scoped
            {
            public:

               Scoped( platform::time::unit timeout);
               Scoped( platform::time::unit timeout, const platform::time::point::type& now);


               template< typename R, typename P>
               Scoped( std::chrono::duration< R, P> timeout)
                  : Scoped( std::chrono::duration_cast< platform::time::unit>( timeout))
               {
               }

               ~Scoped();

            private:
               platform::time::point::type m_old;
               move::Active m_active;

            };

            //! Sets a scoped Deadline.
            //! dtor will 'unset' timeout regardless
            class Deadline
            {
            public:

               Deadline( const platform::time::point::type& deadline, const platform::time::point::type& now);
               Deadline( const platform::time::point::type& deadline);
               Deadline( platform::time::unit timeout, const platform::time::point::type& now);
               Deadline( platform::time::unit timeout);
               ~Deadline();

               Deadline( Deadline&&);
               Deadline& operator = ( Deadline&&);

            private:
               move::Active m_active;
               platform::time::point::type m_old;
            };

         }

         //! Sends the signal to the process
         //!
         //! @return true if the signal was sent
         bool send( strong::process::id pid, code::signal signal);

         struct Set;
         namespace set
         {
            signal::Set filled();
         }

         struct Set
         {
            using type = sigset_t;

            Set();
            Set( type set);
            Set( std::initializer_list< code::signal> signals);

            void add( code::signal signal);
            void remove( code::signal signal);



            friend signal::Set signal::set::filled();

            type set;

            //! @return true if @signal is present in the mask
            bool exists( code::signal signal) const;

            friend std::ostream& operator << ( std::ostream& out, const Set& value);

         private:
            struct filled_t{};
            struct empty_t{};

            Set( filled_t);
            Set( empty_t);

         };

         //! @return current pending signals that has been blocked
         Set pending();

         namespace set
         {
            using type = typename Set::type;

            //! @return a filled mask, that is, all signals are represented
            signal::Set filled();

            //! @param excluded the signals that is excluded from the mask
            //!
            //! @return filled() - excluded
            template< typename... Signals>
            signal::Set filled( code::signal signal, Signals... signals)
            {
               auto set = filled( signals...);
               set.remove( signal);
               return set;
            }

            //! @return an empty set, that is, no signals are represented
            signal::Set empty();

         } // set

         namespace mask
         {
            //! Sets the current signal mask for current thread
            //!
            //! @param mask to replace the current mask
            //! @return the old mask
            signal::Set set( signal::Set mask);

            //! Adds @mask to the current signal mask for current thread
            //!
            //! @param mask to be added to the current mask
            //! @return the old mask
            signal::Set block( signal::Set mask);

            //! Removes @mask from the current signal mask for current thread
            //!
            //! @param mask to be removed from the current mask
            //! @return the old mask
            signal::Set unblock( signal::Set mask);

            //! Blocks all signals to current thread
            //!
            //! @return the old mask
            signal::Set block();

            //! @return the current mask for current thread
            signal::Set current();

         } // mask

         namespace thread
         {
            //! Send signal to thread
            void send( std::thread& thread, code::signal signal);

            //! Send signal to thread
            void send( common::thread::native::type thread, code::signal signal);

            //! Send signal to current thread
            void send( code::signal signal);

            namespace scope
            {
               //! Resets the signal mask on destruction
               struct Reset
               {
                  Reset( signal::Set mask);
                  ~Reset();

                  Reset( Reset&&) = default;
                  Reset& operator = ( Reset&&) = default;

                  const signal::Set& previous() const;

               private:
                  signal::Set m_mask;
                  move::Active m_active;
               };

               //! Sets the signal mask, and
               //! resets original on destruction
               struct Mask : Reset
               {
                  Mask( signal::Set mask);
               };


               struct Block : Reset
               {
                  //! Blocks all signals on construction, and
                  //! resets original on destruction
                  Block();

                  //! Adds @p mask to the current blocking mask,
                  //! @param mask to be added to the blocking set
                  Block( signal::Set mask);

               };

               struct Unblock : Reset
               {
                  //! remove @p mask from the current blocking mask,
                  //! @param mask to be removed from the blocking set
                  Unblock( signal::Set mask);
               };
            } // scope
         } // thread

         //! @returns the most prioritized received signal, if any.
         code::signal received();

         //! Throws if there has been a signal received.
         //! And the signal is NOT blocked in the current
         //! threads signal mask
         //!
         //! @throw subtype to exception::signal::Base
         void handle();

         //! Throws if there has been a signal received.
         //! And the signal is part of the provied signal-sets
         //!
         //! @throw subtype to exception::signal::Base
         void handle( signal::Set set);

         //! Clears all pending signals.
         void clear();

      } // signal
   } // common
} // casual



