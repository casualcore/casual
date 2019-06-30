//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



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
#include <csetjmp>

// time
#include <time.h>



// alarm
#include <unistd.h>

// SSIZE_MAX and others...
#include <climits>

#include <cstring>


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
#include <iosfwd>

namespace casual
{
   using namespace std::literals::chrono_literals;
   
   namespace common
   {
      namespace platform
      {
         namespace size
         {
            using type = long;

            namespace max
            {
               constexpr auto path = PATH_MAX;
            } // max

            namespace by
            {
               namespace value
               {
                  //! max size of type suitable for passing by value ("pods" only)
                  constexpr auto max = 16;
               } // value
            } // by
         } // size

         namespace batch
         {

            namespace transaction
            {
               //! Max number of transaction state updates that will be done
               //! before (forced) persistence store of the updates, could be
               //! stored before though
               constexpr size::type persistence = 100;

               //! Number of xid:s we fetch from the xa-resource
               //! in a batch
               constexpr size::type recover = 8;

            } // transaction

            //! Max number of statistics updates that will be done
            //! before persistence store of the updates...
            constexpr size::type statistics = 1000;

            //! Max number of ipc messages consumed from the queue to cache
            //! (application memory) during a 'flush'
            constexpr size::type flush = 20;

            namespace gateway
            {
               //! Max number of batched metrics before force
               //! send to service-manager
               constexpr size::type metrics = 20;
            } // gateway

            namespace domain
            {
               //! Max number of consumed messages before trying to send
               //! pending messages. 
               constexpr size::type pending = 100;
            } // domain

            namespace queue
            {
               //! Max number of pending updates before 
               //! a persistent write
               constexpr size::type persitent = 100;
            } // queue

            namespace service
            {
               //! Max number of consumed messages before trying to send
               //! pending messages. 
               constexpr size::type pending = 100;

               namespace forward
               {
                  //! Max number of consumed messages before trying to send
                  //! pending messages. 
                  constexpr size::type pending = 100;
               } // forward

               //! max number of 'cached' metrics.
               constexpr size::type metrics = 100;
            } // service

            namespace http
            {
               namespace outbound
               {
                  namespace concurrent
                  {
                     //! max number of 'cached' metrics.
                     constexpr size::type metrics = 100;
                  } // concurrent
               } // outbound
            } // http

         } // batch

         // Some os-specific if-defs?

         namespace directory
         {
            constexpr auto temporary = "/tmp";
         } // directory

         namespace character
         {
            using type = char;
         } // character



         namespace ipc
         {
            namespace native
            {
               //! @attention do not use directly - use strong::ipc::id
               using type = long;
               constexpr type invalid = -1;
            } // native

            namespace message
            {
               using type = long;
            }

            namespace transport
            {

#ifdef __APPLE__
               // OSX has very tight limits on IPC
               constexpr size::type size = 1024 * 1;
#else
               constexpr size::type size = 1024 * 8;
#endif

            } // transport

         } // ipc

         namespace socket
         {
            namespace native
            {
               //! @attention do not use directly - use strong::socket::id
               using type = int;
               constexpr type invalid = -1;
            } // native
         } // socket

         namespace tcp
         {
            namespace message
            {
               namespace max
               {
                  constexpr size::type size = SSIZE_MAX;
                  static_assert( size > 0, "tcp::message::max::size overflow");

               } // max
               constexpr size::type size = 1024 * 16;

               static_assert( size <= max::size, "requested tcp message size is to big");
            } // message

            namespace listen
            {
               //! backlog for listen
               constexpr int backlog = 10;               
            } // listen

         } // tcp

         namespace file
         {
            namespace descriptor
            {
               namespace native
               {
                  //! @attention do not use directly - use strong::file::descriptor::id
                  using type = int;
                  constexpr type invalid = -1;
               } // native
            } // descriptor
         } // file

         namespace process
         {
            namespace native
            {
               //! @attention do not use directly - use strong::process::id
               using type = pid_t;
            } // native
         } // process

         namespace queue
         {
            namespace native
            {
               //! @attention do not use directly - use strong::queue::id
               using type = size::type;
               constexpr type invalid = 0;
            } // native
         } // process

         //
         // uuid
         //
         namespace uuid
         {
            using type = uuid_t;
         } // uuid

         namespace jump
         {
            using buffer = std::jmp_buf;
         } // jump


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
            namespace native
            {
               using type = int;   
            } // native
         } // signal


         namespace resource
         {
            namespace native
            {
               //! @attention do not use directly - use strong::resource::id
               using type = int;
               constexpr type invalid = 0;
            } // native
         } // resource

         namespace binary
         {
            using type = std::vector< char>;
            using pointer = decltype( std::declval< binary::type>().data());

            namespace value
            {
               using type = std::remove_reference_t< decltype( std::declval< binary::type>().at( 0))>;

            } // value

            namespace immutable
            {
               using pointer = decltype( std::declval< const binary::type>().data());
            } // immutable

            namespace size
            {
               //! The common type used to represent sizes (especially in buffers)
               using type = platform::size::type;

            } // size

         } // binary

         namespace buffer
         {
            namespace raw
            {
               using type = binary::pointer;

               namespace size
               {
                  using type = platform::size::type;
               } // size


               namespace immutable
               {
                  using type = binary::immutable::pointer;
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
               // If we need to_time_t, we need to use system_clock
               using type = std::chrono::system_clock;
            } // clock

            namespace point
            {
               using type = clock::type::time_point;
            } // point

            using unit = point::type::duration;
            
            namespace serialization
            {
               using unit = std::chrono::nanoseconds;
            } // serialization

            namespace limit
            {
               constexpr auto zero() noexcept { return std::chrono::duration_cast< time::unit>( std::chrono::nanoseconds::zero());}
               constexpr auto max() noexcept { return std::chrono::duration_cast< time::unit>( std::chrono::nanoseconds::max());}
               constexpr auto min() noexcept { return std::chrono::duration_cast< time::unit>( std::chrono::nanoseconds::min());}
            } // limit
         } // time


         namespace descriptor
         {
            //! Call-descriptor type
            using type = int;
         } // descriptor



      } // platform
   } // common

} // casual

#if __GNUC__ > 4 || __clang_major__ > 4
#define CASUAL_OPTION_UNUSED __attribute__((unused))
#else
#define CASUAL_OPTION_UNUSED
#endif


