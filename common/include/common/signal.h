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

namespace casual
{

	namespace common
	{
		namespace signal
		{
		   using set_type = sigset_t;

		   namespace type
		   {

		      static const auto alarm = platform::cSignal_Alarm;
		      static const auto interupt = platform::cSignal_Interupt;
		      static const auto kill = platform::cSignal_Kill;
		      static const auto quit = platform::cSignal_Quit;
		      static const auto child = platform::cSignal_ChildTerminated;
		      static const auto terminate = platform::cSignal_Terminate;
		      static const auto user = platform::cSignal_UserDefined;

		      using type = decltype( terminate);


		      std::string string( type signal);

         } // type


			//!
			//! Throws if there has been a signal received.
			//!
			//! @throw subtype to exception::signal::Base
			//!
			void handle();


			//!
			//! Clears all pending signals.
			//!
			void clear();


			namespace alarm
			{
				class Scoped
				{
				public:

					Scoped( std::chrono::microseconds timeout);

	            template< typename R, typename P>
	            Scoped( std::chrono::duration< R, P> timeout)
	               : Scoped( std::chrono::duration_cast< std::chrono::microseconds>( timeout))
	            {
	            }

					~Scoped();

				private:
					std::chrono::microseconds m_old;

				};

				void set( platform::time_point when);

			}

			namespace timer
			{
			   std::chrono::microseconds set( std::chrono::microseconds offset);

			   template< typename R, typename P>
			   void set( std::chrono::duration< R, P> offset)
			   {
			      set( std::chrono::duration_cast< std::chrono::microseconds>( offset));
			   }

			   std::chrono::microseconds unset();

			}





			//!
			//! Sends the signal to the process
			//!
			//! @return true if the signal was sent
			//!
			bool send( platform::pid_type pid, type::type signal);

			//!
			//! Blocks a given signal
			//!
			void block( type::type signal);

			//!
         //! Unblock a given signal
         //!
			void unblock( type::type signal);

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
