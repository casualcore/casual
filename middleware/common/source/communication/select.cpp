//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/select.h"
#include "common/communication/device.h"

#include "common/signal.h"
#include "common/result.h"
#include "common/memory.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include "casual/assert.h"

namespace casual
{
   using namespace common;

   namespace common::communication::select
   {

      namespace directive
      {
         void Set::add( strong::file::descriptor::id descriptor) noexcept
         {
            CASUAL_ASSERT( descriptor.value() >= 0 && descriptor.value() < FD_SETSIZE);

            if( ! algorithm::find( m_descriptors, descriptor))
            {
               m_descriptors.push_back( descriptor);
               m_highest = std::max( m_highest, descriptor);
            }
         }

         void Set::remove( strong::file::descriptor::id descriptor) noexcept
         {
            if( auto found = algorithm::find( m_descriptors, descriptor))
            {
               m_descriptors.erase( std::begin( found));

               if( m_highest != descriptor)
                  return;

               if( auto found = algorithm::max( m_descriptors))
                  m_highest = *found;
               else
                  m_highest = {};
            }
         }

      } // directive


      namespace dispatch
      {
         namespace detail
         {
            namespace local
            {
               namespace
               {
                  template< typename T> 
                  void fd_zero( T& value)
                  {
#ifdef __APPLE__
                     // error: The bzero() function is obsoleted by memset() [clang-analyzer-security.insecureAPI.bzero,-warnings-as-errors]
                     value = T{};
#else
                     FD_ZERO( &value);
#endif 
                  }
                  
                  //! takes care of atomic signals...
                  void select( strong::file::descriptor::id highest, ::fd_set* read, ::fd_set* write)
                  {
                     Trace trace{ "common::communication::select::dispatch::detail::local::select"};
                     // block all signals
                     signal::thread::scope::Block block;

                     // check pending signals
                     if( signal::pending( block.previous()))
                        code::raise::error( code::casual::interrupted);

                     posix::result( 
                        // will set previous signal mask atomically 
                        // note: first argument: nfds is the highest-numbered file descriptor in any of the three sets, plus 1.
                        ::pselect( highest.value() + 1, read, write, nullptr, nullptr, &block.previous().set));
                  }

               } // <unnamed>
            } // local

            directive::Ready select( const Directive& directive)
            {
               Trace trace{ "common::communication::select::dispatch::detail::select"};
               log::line( verbose::log, "directive: ", directive);

               ::fd_set read;
               ::fd_set write;
               local::fd_zero( read);
               local::fd_zero( write);

               auto set_set = []( auto&& descriptors, auto& set)
               {
                  for( auto descriptor : descriptors)
                  {
                     assert( descriptor);
                     FD_SET( descriptor.value(), &set);
                  }
               };

               set_set( directive.read.descriptors(), read);
               set_set( directive.write.descriptors(), write);

               // takes care of atomic signals...
               local::select( directive.highest(), &read, &write);

               auto filter_ready = []( auto&& descriptors, auto& set)
               {
                  return algorithm::filter( descriptors, [&set]( auto& descriptor)
                  {
                     return FD_ISSET( descriptor.value(), &set) != 0;
                  });
               };

               return directive::Ready{
                  filter_ready( directive.read.descriptors(), read),
                  filter_ready( directive.write.descriptors(), write),
               };
            }
         } // detail

      } // dispatch

      namespace block
      {
         void read( strong::file::descriptor::id descriptor)
         {
            ::fd_set read;
            dispatch::detail::local::fd_zero( read);

            assert( descriptor);
            FD_SET( descriptor.value(), &read);

            dispatch::detail::local::select( descriptor, &read, nullptr);
         }

         void write( strong::file::descriptor::id descriptor)
         {
            ::fd_set write;
            dispatch::detail::local::fd_zero( write);

            assert( descriptor);
            FD_SET( descriptor.value(), &write);

            dispatch::detail::local::select( descriptor, nullptr, &write);
         }
      } // block


   } // common::communication::select
} // casual