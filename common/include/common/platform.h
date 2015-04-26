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

// uint64_t
#include <cstdint>


//uuid
#include <uuid/uuid.h>

// longjump
#include <setjmp.h>

// time
#include <time.h>

// syslog
#include <syslog.h>



// alarm
#include <unistd.h>


#include <string.h>


#ifdef __APPLE__
#include <syslimits.h>
#else

#endif


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
         //! The common type used to represent sizes (especially in buffers)
         //!
		   typedef uint64_t binary_size_type;


		   namespace batch
         {
	         //!
	         //! Max number of transaction state updates that will be done
	         //! before (forced) persistence store of the updates, though could be
	         //! stored before though
	         //!
	         constexpr std::size_t transaction = 100;

	         //!
	         //! Max number of statistics updates that will be done
	         //! before persistence store of the updates...
	         //!
	         constexpr std::size_t statistics = 1000;

         } // batch



			//
			// Some os-specific if-defs?
			//

		   namespace size
         {
		      namespace max
            {
		         constexpr auto path = PATH_MAX;
            } // max
         } // size



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


			typedef int signal_type;



			namespace resource
         {
            typedef int id_type;

         } // resource



			typedef std::vector< char> binary_type;

			using raw_buffer_type = char*;
			using raw_buffer_size = long;
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
   void casual_marshal_value( const common::platform::time_point& value, M& marshler)
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
