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
                     memory::clear( value); 
#else
                     FD_ZERO( &value);
#endif 
                  }

                  template< typename T>
                  auto& print_fd( std::ostream& out, const T& set, traits::priority::tag< 0>) { return out;}

                  template< typename T>
                  auto print_fd( std::ostream& out, const T& set, traits::priority::tag< 1>) 
                     -> decltype( stream::write( out, set.fds_bits))
                  {
                     return stream::write( out, set.fds_bits);
                  }

               } // <unnamed>
            } // local
            namespace directive
            {
               Set::Set()
               {
                  local::fd_zero( m_set);
               }
          
               void Set::add( strong::file::descriptor::id descriptor)
               {
                  FD_SET( descriptor.value(), &m_set);
               }

               void Set::remove( strong::file::descriptor::id descriptor)
               {
                  FD_CLR( descriptor.value(), &m_set);
               }

               bool Set::ready( strong::file::descriptor::id descriptor) const
               {
                  return FD_ISSET( descriptor.value(), &m_set);
               }

               std::ostream& operator << ( std::ostream& out, const Set& value)
               {
                  if( out)
                     local::print_fd( out, value.m_set, traits::priority::tag< 1>{});

                  return out;
               }
  
            } // directive

            namespace dispatch
            {
               namespace detail
               {
                  Directive select( const Directive& directive)
                  {
                     Trace trace{ "common::communication::select::dispatch::detail::select"};
                     log::line( verbose::log, "directive: ", directive);

                     auto result = directive;

                     // block all signals, just local in this scope, not when we do the dispatch 
                     // further down.
                     signal::thread::scope::Block block;

                     // check pending signals
                     if( signal::pending( block.previous()))
                        code::raise::log( code::casual::interupted);

                     log::line( verbose::log, "pselect - blocked signals: ", block.previous());
                    
                     posix::result( 
                         // will set previous signal mask atomically 
                        ::pselect( FD_SETSIZE, result.read.native(), nullptr, nullptr, nullptr, &block.previous().set));

                     log::line( verbose::log, "result: ", result);
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
                  Directive directive;
                  directive.read.add( descriptor);
                  dispatch::detail::select( directive);
               }
            } // block

         } // select
      } // communication
   } // common
} // casual