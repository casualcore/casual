//!
//! casual_utility_signal.h
//!
//! Created on: May 6, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_SIGNAL_H_
#define CASUAL_UTILITY_SIGNAL_H_


#include <cstddef>

#include "common/platform.h"
#include "common/move.h"
#include "common/internal/log.h"
#include "common/algorithm.h"
#include "common/thread.h"


#include <atomic>
#include <thread>

// signal
#include <signal.h>

namespace casual
{

   namespace common
   {
      namespace signal
      {

         enum class Type : platform::signal::type
         {
            alarm = SIGALRM,
            interupt = SIGINT,
            kill = SIGKILL,
            quit = SIGQUIT,
            child = SIGCHLD,
            terminate = SIGTERM,
            user = SIGUSR1,
            pipe = SIGPIPE,
         };

         std::ostream& operator << ( std::ostream& out, signal::Type signal);

		   namespace type
         {
            std::string string( Type signal);
            std::string string( platform::signal::type signal);

         } // type



			//!
			//! Throws if there has been a signal received.
			//! And the signal is NOT blocked in the current
			//! threads signal mask
			//!
			//! @throw subtype to exception::signal::Base
			//!
			void handle();

			//!
			//! Clears all pending signals.
			//!
			void clear();


			namespace timer
			{
			   //!
			   //! Sets a timeout.
			   //!
			   //! @param offset when the timer kicks in.
			   //! @returns previous timeout.
			   //!
			   //! @note zero and negative offset will trigger a signal directly
			   //! @note std::chrono::microseconds::min() has special meaning and will not set any
			   //! timeout and will unset current timeout, if any.
			   //!
			   std::chrono::microseconds set( std::chrono::microseconds offset);

			   template< typename R, typename P>
			   std::chrono::microseconds set( std::chrono::duration< R, P> offset)
			   {
			      return set( std::chrono::duration_cast< std::chrono::microseconds>( offset));
			   }

			   //!
			   //! @return current timeout, std::chrono::microseconds::min() if there isn't one
			   //!
			   std::chrono::microseconds get();

            //!
            //! Unset current timeout, if any.
            //!
            //! @return previous timeout, std::chrono::microseconds::min() if there wasn't one
            //!
			   std::chrono::microseconds unset();


			   //!
			   //! Sets a scoped timout.
			   //! dtor will 'reset' previous timeout, if any. Hence enable nested timeouts.
			   //!
			   //! @note std::chrono::microseconds::min() has special meaning and will not set any
            //! timeout and will unset current timeout, if any (that will be reset by dtor)
			   //!
            class Scoped
            {
            public:

			      Scoped( std::chrono::microseconds timeout);
               Scoped( std::chrono::microseconds timeout, const platform::time_point& now);


               template< typename R, typename P>
               Scoped( std::chrono::duration< R, P> timeout)
                  : Scoped( std::chrono::duration_cast< std::chrono::microseconds>( timeout))
               {
               }

               ~Scoped();

            private:
               platform::time_point m_old;
               move::Moved m_moved;

            };

            //!
            //! Sets a scoped Deadline.
            //! dtor will 'unset' timeout regardless
            //!
            class Deadline
            {
            public:

               Deadline( const platform::time_point& deadline, const platform::time_point& now);
               Deadline( const platform::time_point& deadline);
               Deadline( std::chrono::microseconds timeout, const platform::time_point& now);
               Deadline( std::chrono::microseconds timeout);
               ~Deadline();

               Deadline( Deadline&&);
               Deadline& operator = ( Deadline&&);

            private:
               move::Moved m_moved;
               platform::time_point m_old;
            };

			}


			//!
			//! Sends the signal to the process
			//!
			//! @return true if the signal was sent
			//!
			bool send( platform::pid::type pid, Type signal);

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
			   Set( std::initializer_list< Type> signals);

            void add( Type signal);
            void remove( Type signal);



            friend signal::Set signal::set::filled();

            type set;

            //!
            //! @return true if @signal is present in the mask
            //!
            bool exists( Type signal) const;

            friend std::ostream& operator << ( std::ostream& out, const Set& value);

         private:
            struct filled_t{};
            struct empty_t{};

            Set( filled_t);
            Set( empty_t);

         };

         //!
         //! @return current pending signals that has been blocked
         //!
         Set pending();


			namespace set
         {
			   using type = typename Set::type;

            //!
            //! @return a filled mask, that is, all signals are represented
            //!
			   signal::Set filled();

            //!
            //!
            //! @param excluded the signals that is excluded from the mask
            //!
            //! @return filled() - excluded
            //!
            //Mask filled( std::initializer_list< Type> excluded);
			   signal::Set filled( const std::vector< Type>& excluded);


            //!
            //! @return an empty set, that is, no signals are represented
            //!
			   signal::Set empty();

         } // set

			namespace mask
         {
			   //!
			   //! Sets the current signal mask for current thread
			   //!
			   //! @param mask to replace the current mask
			   //! @return the old mask
			   //!
			   signal::Set set( signal::Set mask);

            //!
            //! Adds @mask to the current signal mask for current thread
            //!
            //! @param mask to be added to the current mask
            //! @return the old mask
            //!
			   signal::Set block( signal::Set mask);

            //!
            //! Removes @mask from the current signal mask for current thread
            //!
            //! @param mask to be removed from the current mask
            //! @return the old mask
            //!
			   signal::Set unblock( signal::Set mask);

            //!
            //! Blocks all signals to current thread
            //!
            //! @return the old mask
            //!
			   signal::Set block();



            //!
            //! @return the current mask for current thread
            //!
			   signal::Set current();

         } // mask

			namespace thread
         {



			   //!
			   //! Send signal to thread
			   //!
			   void send( std::thread& thread, Type signal);


            //!
            //! Send signal to thread
            //!
            void send( common::thread::native::type thread, Type signal);

            //!
            //! Send signal to current thread
            //!
            void send( Type signal);


			   namespace scope
            {
               //!
               //! Resets the signal mask on destruction
               //!
			      struct Reset
			      {
			         Reset( signal::Set mask);
			         ~Reset();

			         Reset( Reset&&) = default;
			         Reset& operator = ( Reset&&) = default;

               private:
			         signal::Set m_mask;
                  move::Moved m_moved;
			      };

               //!
               //! Sets the signal mask, and
               //! resets original on destruction
               //!
               struct Mask : Reset
               {
                  Mask( signal::Set mask);
               };


               struct Block : Reset
               {
                  //!
                  //! Blocks all signals on construction, and
                  //! resets original on destruction
                  //!
                  Block();

                  //!
                  //! Adds @p mask to the current blocking mask,
                  //! @param mask to be added to the blocking set
                  //!
                  Block( signal::Set mask);

               };

               struct Unblock : Reset
               {

                  //!
                  //! remove @p mask from the current blocking mask,
                  //! @param mask to be removed from the blocking set
                  //!
                  Unblock( signal::Set mask);

               };


            } // scope


         } // thread



		} // signal
	} // common
} // casual



#endif /* CASUAL_UTILITY_SIGNAL_H_ */
