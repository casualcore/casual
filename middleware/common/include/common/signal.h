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
#include "common/algorithm/compare.h"
#include "common/functional.h"
#include "common/thread.h"
#include "common/execute.h"


#include <atomic>
#include <thread>
#include <cstddef>

// signal
#include <signal.h>

namespace casual
{
   namespace common::signal
   {
      namespace current
      {
         //! @returns the current number of pending signals
         //!
         //! @note only (?) for unittest
         platform::size::type pending();
      } // current


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
      //Set pending();

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


      namespace callback
      {
         namespace detail
         {
            void registration( code::signal signal, common::function< void()> callback);

            struct Replace
            {
               code::signal signal;
               std::vector< common::function< void()>> callbacks;
            };
            Replace replace( Replace wanted);
            
         } // detail

         template< code::signal signal, typename T>
         auto registration( T&& callback)
         {
            static_assert( algorithm::compare::any( signal, 
               code::signal::alarm, 
               code::signal::hangup,
               code::signal::user,
               code::signal::child), "not a valid signal for callback");

            return detail::registration( signal, std::forward< T>( callback));
         }

         namespace scoped
         {
            template< code::signal signal, typename T>
            auto replace( T&& callback)
            {
               static_assert( algorithm::compare::any( signal, 
                  code::signal::alarm, 
                  code::signal::hangup,
                  code::signal::user,
                  code::signal::child), "not a valid signal for callback");

               detail::Replace value;
               value.signal = signal;
               value.callbacks.emplace_back( std::forward< T>( callback));

               return execute::scope( [ old = detail::replace( std::move( value))]()
               {
                  detail::replace( std::move( old));
               });
            }
         } // scoped
      } // callback

      //! Dispatch pending signal to callbacks, that is not blocked by current signal mask.
      //! if no callbacks been registred an exception will be thrown
      //!
      //! @throw subtype to exception::signal::exception
      void dispatch();

      //! Dispatch pending signal to callbacks, that is not blocked by the provided signal mask
      //! if no callbacks been registred an exception will be thrown
      //!
      //! @throw subtype to exception::signal::exception
      void dispatch( signal::Set mask);

      //! @return true if there are signals that hasn't been consumed/dispatched on.
      bool pending( signal::Set mask);

      //! Clears all pending signals, only for unittests...
      void clear();


   } // common::signal
} // casual



