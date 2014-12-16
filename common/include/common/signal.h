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

         void handle( const std::vector< signal::Type>& exclude);


			//!
			//! Clears all pending signals.
			//!
			void clear();


			namespace timer
			{
			   std::chrono::microseconds set( std::chrono::microseconds offset);

			   template< typename R, typename P>
			   std::chrono::microseconds set( std::chrono::duration< R, P> offset)
			   {
			      return set( std::chrono::duration_cast< std::chrono::microseconds>( offset));
			   }


			   std::chrono::microseconds get();

			   std::chrono::microseconds unset();


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



		} // signal
	} // common
} // casual



#endif /* CASUAL_UTILITY_SIGNAL_H_ */
