#include "common/communication/select.h"

#include "common/signal.h"
#include "common/result.h"
#include "common/memory.h"

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
  
            } // directive

            namespace dispatch
            {
               namespace detail
               {
                  Directive select( const Directive& directive)
                  {
                     auto result = directive;

                     Trace trace{ "common::communication::select::dispatch::detail::select"};

                     // block all signals, just local in this scope, not when we do the dispatch 
                     // further down.
                     signal::thread::scope::Block block;

                     // check pending signals
                     signal::handle( block.previous());

                     log::line( verbose::log, "pselect - blocked signals: ", block.previous());

                    
                     posix::result( 
                         // will set previous signal mask atomically 
                        ::pselect( FD_SETSIZE, result.read.native(), nullptr, nullptr, nullptr, &block.previous().set),
                        // we need to pass previous set to be able to dispatch on signals
                        block.previous().set);

                     return result;
                }
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