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

// tcp
#include <sys/socket.h>


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

// SSIZE_MAX and others...
#include <climits>

#include <string.h>


#ifdef __APPLE__
#include <limits.h>
#else
#include <linux/limits.h>
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

		   namespace batch
         {
	         //!
	         //! Max number of transaction state updates that will be done
	         //! before (forced) persistence store of the updates, could be
	         //! stored before though
	         //!
	         constexpr std::size_t transaction() { return 100;}

	         //!
	         //! Max number of statistics updates that will be done
	         //! before persistence store of the updates...
	         //!
	         constexpr std::size_t statistics() { return 1000;}

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


		   namespace ipc
         {
		      namespace id
            {
               using type = long;
            } // id

            namespace message
            {

               using type = long;

#ifdef __APPLE__
               //
               // OSX has very tight limits on IPC
               //
               constexpr std::size_t size = 1024;
#else
               constexpr std::size_t size = 1024 * 8;
#endif

            } // message

         } // ipc


			namespace tcp
         {
            namespace message
            {
               namespace max
               {
                  constexpr std::size_t size = SSIZE_MAX;
               } // max
               constexpr std::size_t size = 1024 * 16;

               static_assert( size <= max::size, "requested tcp message size is to big");

            } // message

            namespace descriptor
            {
               using type = int;
            } // handle

         } // tcp

			namespace pid
         {
			   using type = pid_t;
         } // pid

			//
			// uuid
			//
			namespace uuid
         {
			   using type = uuid_t;

			   namespace string
            {
               using type = char[ 37];
            } // string

         } // uuid

			//
			// long jump
			//
			using long_jump_buffer_type = jmp_buf;


			namespace flag
         {
			   enum class ipc
			   {
			      no_wait = IPC_NOWAIT
			   };

			   enum class tcp
			   {
			      no_wait = MSG_DONTWAIT
			   };

            enum class msg : int
            {
#ifdef __APPLE__
               no_signal = SO_NOSIGPIPE,
#else
               no_signal = MSG_NOSIGNAL,
#endif
            };

            template< typename F>
            constexpr auto value( F flag) -> typename std::underlying_type< F>::type
            {
               return static_cast< typename std::underlying_type< F>::type>( flag);
            }

         } // flags

			namespace signal
         {
			   using type = int;

         } // signal


			namespace resource
         {
			   namespace id
            {
			      using type = int;
            } // id
         } // resource


			namespace binary
         {
            using type = std::vector< char>;

            namespace size
            {

               //!
               //! The common type used to represent sizes (especially in buffers)
               //!
               using type = uint64_t;

            } // size

         } // binary

			namespace buffer
         {
            namespace raw
            {
               using type = char*;

               namespace size
               {
                  using type = long;
               } // size


               namespace immutable
               {
                  using type = const char*;
               } // immutable

               inline type external( immutable::type buffer)
               {
                  return const_cast< type>( buffer);
               }

            } // raw
         } // buffer


			namespace time
         {
            namespace clock
            {
               // TODO: change to: typedef std::chrono::steady_clock clock_type;
               // When clang has to_time_t for steady_clock
               using type = std::chrono::system_clock;
            } // clock

            namespace point
            {
               using type = clock::type::time_point;
            } // point

         } // time


         namespace descriptor
         {
            //!
            //! Call-descriptor type
            //!
            using type = int;
         } // descriptor



		} // platform
	} // common

	//!
   //! Overload for time_type
   //!
   //! @{

   template< typename R, typename P, typename M>
   void casual_marshal_value( const std::chrono::duration< R, P>& value, M& marshler)
   {
      marshler << std::chrono::duration_cast< std::chrono::microseconds>( value).count();
   }

   template< typename R, typename P, typename M>
   void casual_unmarshal_value( std::chrono::duration< R, P>& value, M& unmarshler)
   {
      std::chrono::microseconds::rep representation;

      unmarshler >> representation;
      value = std::chrono::microseconds( representation);
   }


   template< typename M>
   void casual_marshal_value( const common::platform::time::point::type& value, M& marshler)
   {
      const auto time = value.time_since_epoch().count();
      marshler << time;
   }

   template< typename M>
   void casual_unmarshal_value( common::platform::time::point::type& value, M& unmarshler)
   {
      common::platform::time::point::type::rep representation;
      unmarshler >> representation;
      value = common::platform::time::point::type( common::platform::time::point::type::duration( representation));
   }
   //! @}



} // casual

#if __GNUC__ > 4 || __clang_major__ > 4
#define CASUAL_OPTION_UNUSED __attribute__((unused))
#else
#define CASUAL_OPTION_UNUSED
#endif

#endif /* CASUAL_UTILITY_PLATFORM_H_ */
