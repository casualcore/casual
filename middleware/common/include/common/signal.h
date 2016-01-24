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


#include <thread>

// signal
#include <signal.h>

namespace casual
{

   namespace common
   {
      namespace signal
      {
         using set_type = sigset_t;

         enum Type : platform::signal_type
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

		   namespace type
         {
            using type = platform::signal_type;

            std::string string( type signal);

         } // type


			//!
			//! Throws if there has been a signal received.
			//!
			//! @throw subtype to exception::signal::Base
			//!
			void handle();


			enum Filter
			{
			   exclude_none = 0,
			   exclude_alarm = 1,
			   exclude_child_terminate = 2,
			   exclude_terminate = 4
			};

         void handle( Filter exclude);


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
			bool send( platform::pid_type pid, type::type signal);


			namespace thread
         {
			   //!
			   //! Send signal to thread
			   //!
			   void send( std::thread& thread, type::type signal);

			   //!
			   //! Blocks all signals to current thread
			   //!
			   set_type block();

			   set_type mask( set_type set);

			   namespace scope
            {
			      //!
			      //! Blocks all signals on construction, and
			      //! resets original on destruction
			      //!
               struct Block
               {
                  Block();
                  ~Block();

               private:
                  set_type m_set;
               };

            } // scope


         } // thread

			namespace scope
			{
			   //! @remark This is not thread-safe
			   struct Ignore
			   {
			      Ignore( type::type signal);
               ~Ignore();

			   private:
               const type::type m_signal;
               const sighandler_t m_previous;
			   };

			} // scope


		} // signal
	} // common
} // casual



#endif /* CASUAL_UTILITY_SIGNAL_H_ */
