#include "common/communication/select.h"
#include "common/communication/device.h"

#include "common/signal.h"
#include "common/result.h"
#include "common/memory.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace select
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

               } // <unnamed>
            } // local
            namespace directive
            {
               namespace native
               {
            
                  Set::Set()
                  {
                     local::fd_zero( m_set);
                  }

                  void Set::add( strong::file::descriptor::id descriptor) noexcept 
                  { 
                     assert( descriptor);
                     FD_SET( descriptor.value(), &m_set);
                  }
                  void Set::remove( strong::file::descriptor::id descriptor) noexcept
                  {
                     assert( descriptor);
                     FD_CLR( descriptor.value(), &m_set);
                  }

                  bool Set::ready( strong::file::descriptor::id descriptor) const noexcept
                  {
                     assert( descriptor);
                     return FD_ISSET( descriptor.value(), &m_set);
                  }
                     
               } // native

               
          
               void Set::add( strong::file::descriptor::id descriptor) noexcept
               {
                  m_descriptors.push_back( descriptor);
                  m_set.add( descriptor);
               }

               void Set::remove( strong::file::descriptor::id descriptor) noexcept
               {
                  if( auto found = algorithm::find( m_descriptors, descriptor))
                  {
                     m_descriptors.erase( std::begin( found)),
                     m_set.remove( descriptor);
                  }
               }

               bool Set::ready( strong::file::descriptor::id descriptor) const noexcept
               {
                  return m_set.ready( descriptor);
               }
  
            } // directive

            namespace dispatch
            {
               namespace detail
               {
                  directive::native::Set select( const directive::native::Set& read)
                  {
                     Trace trace{ "common::communication::select::dispatch::detail::select"};

                     auto result = read;

                     // block all signals
                     signal::thread::scope::Block block;

                     // check pending signals
                     if( signal::pending( block.previous()))
                        code::raise::log( code::casual::interupted);
                    
                     posix::result( 
                         // will set previous signal mask atomically 
                        ::pselect( FD_SETSIZE, result.native(), nullptr, nullptr, nullptr, &block.previous().set));

                     return result;
                  }

                  namespace handle
                  {
                     void error()
                     {
                        Trace trace{ "common::communication::select::dispatch::detail::handle::error"};
                        device::handle::error();
                     }
                  } // handle

               } // detail

            } // dispatch

            namespace block
            {
               void read( strong::file::descriptor::id descriptor)
               {
                  directive::native::Set read;
                  read.add( descriptor);
                  dispatch::detail::select( read);
               }
            } // block

         } // select
      } // communication
   } // common
} // casual