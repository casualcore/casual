//!
//! casual_utility_platform.h
//!
//! Created on: Jun 14, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_PLATFORM_H_
#define CASUAL_UTILITY_PLATFORM_H_


// ipc
#include <sys/msg.h>
#include <sys/ipc.h>

// size_t
#include <cstddef>

//uuid
#include <uuid/uuid.h>

// longjump
#include <setjmp.h>

// time
#include <time.h>

// syslog
#include <syslog.h>

// signal
#include <signal.h>

// alarm
#include <unistd.h>


#include <string.h>


//
// std
//
#include <string>
#include <vector>
#include <chrono>

namespace casual
{
	namespace common
	{
		namespace platform
		{

		   //
		   // Some sizes
		   //


		   //!
		   //! Max number of transaction state updates that will be done
		   //! before persistence store of the updates...
		   //!
		   const std::size_t transaction_batch = 100;

		   //!
         //! Max number of statistics updates that will be done
         //! before persistence store of the updates...
         //!
		   const std::size_t statistics_batch = 1000;

			//
			// Some os-specific if-defs?
			//

			//
			// System V start
			//

			//
			// ipc
			//
			typedef int queue_id_type;
			typedef long message_type_type;


			// TODO: bigger!
			// const std::size_t message_size = 2048;
			 const std::size_t message_size = 1024;

			//
			// uuid
			//
			typedef uuid_t uuid_type;
			typedef char uuid_string_type[ 37];

			typedef pid_t pid_type;

			//
			// long jump
			//
			typedef jmp_buf long_jump_buffer_type;

			//
			// time
			//
			typedef time_t seconds_type;


			enum ipc_flags
			{
				cIPC_NO_WAIT = IPC_NOWAIT
			};


			enum log_category
			{
				//! system is unusable
				cLOG_emergency = LOG_EMERG,
				//! action must be taken immediately
				cLOG_alert = LOG_ALERT,
				//! critical conditions
				cLOG_critical = LOG_CRIT,
				//! error conditions
				cLOG_error= LOG_ERR,
				//! warning conditions
				cLOG_warning = LOG_WARNING,
				//! normal, but significant, condition
				cLOG_notice = LOG_NOTICE,
				//! informational message
				cLOG_info = LOG_INFO,
				//! debug-level message
				cLOG_debug = LOG_DEBUG
			};

			typedef int signal_type;

			static const signal_type cSignal_Alarm = SIGALRM;
			static const signal_type cSignal_Terminate = SIGTERM;
			static const signal_type cSignal_Kill = SIGKILL;
			static const signal_type cSignal_Quit = SIGQUIT;
			static const signal_type cSignal_Interupt = SIGINT;
			static const signal_type cSignal_ChildTerminated = SIGCHLD;
			static const signal_type cSignal_UserDefined = SIGUSR1;





			namespace resource
         {
            typedef int id_type;

         } // resource



			typedef std::vector< char> binary_type;

         //typedef char* raw_buffer_type;
			using raw_buffer_type = char*;
			using const_raw_buffer_type = const char*;

         inline raw_buffer_type public_buffer( const_raw_buffer_type buffer)
         {
            return const_cast< raw_buffer_type>( buffer);
         }

         // TODO: change to: typedef std::chrono::steady_clock clock_type;
         // When clang has to_time_t for steady_clock
         typedef std::chrono::system_clock clock_type;


         typedef clock_type::time_point time_point;


         //!
         //! Call-descriptor type
         //!
         using descriptor_type = int;




		} // platform
	} // common

	//!
   //! Overload for time_type
   //!
   //! @{
   template< typename M>
   void casual_marshal_value( common::platform::time_point& value, M& marshler)
   {
      auto time = value.time_since_epoch().count();
      marshler << time;
   }

   template< typename M>
   void casual_unmarshal_value( common::platform::time_point& value, M& unmarshler)
   {
      common::platform::time_point::rep representation;
      unmarshler >> representation;
      value = common::platform::time_point( common::platform::time_point::duration( representation));
   }
   //! @}

} // casual



#endif /* CASUAL_UTILITY_PLATFORM_H_ */
