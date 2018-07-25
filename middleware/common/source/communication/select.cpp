#include "common/communication/select.h"

#include "common/signal.h"
#include "common/result.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace select
         {
            namespace directive
            {
               Set::Set()
               {
                  FD_ZERO( &m_set);
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


/*
            namespace dispatch
            {
               namespace detail
               {      
                  void pump( const Directive& origin, const std::vector< Dispatch>& readers)
                  {
                     Trace trace{ "common::communication::select::dispatch::block"};

                     while( true)
                     {
                        // pselect modifies fd-sets so we need to reinitialize them every time
                        auto read = origin.read;

                        // multiplex
                        {
                           Trace trace{ "common::communication::select::dispatch::block multiplex"};

                           // block all signals, just local in this scope, not when we do the dispatch 
                           // further down.
                           signal::thread::scope::Block block;

                           // check pending signals
                           signal::handle( block.previous());

                           log::line( verbose::log, "pselect - blocked signals: ", block.previous());

                           // will set previous signal mask atomically 
                           posix::result( 
                              //::pselect( directive.max().value() + 1, directive.native_read(), nullptr, nullptr, nullptr, &block.previous().set));
                              ::pselect( FD_SETSIZE, read.native(), nullptr, nullptr, nullptr, &block.previous().set));
                        }

                        for( const auto& reader : readers)
                        {
                           for( auto descriptor : reader.descriptors())
                           {
                              if( FD_ISSET( descriptor.value(), read.native()))
                              {
                                 reader( descriptor);
                              }
                           }
                        }
                     }
                  }
               } // detail
            } // dispatch

*/

            namespace dispatch
            {
               namespace detail
               {
                  Directive select( const Directive& directive)
                  {
                     auto result = directive;

                     Trace trace{ "common::communication::select::dispatch::block multiplex"};

                     // block all signals, just local in this scope, not when we do the dispatch 
                     // further down.
                     signal::thread::scope::Block block;

                     // check pending signals
                     signal::handle( block.previous());

                     log::line( verbose::log, "pselect - blocked signals: ", block.previous());

                     // will set previous signal mask atomically 
                     posix::result( 
                        //::pselect( directive.max().value() + 1, directive.native_read(), nullptr, nullptr, nullptr, &block.previous().set));
                        ::pselect( FD_SETSIZE, result.read.native(), nullptr, nullptr, nullptr, &block.previous().set));

                     return result;
                }
               } // detail

            } // dispatch

         } // select
      } // communication
   } // common
} // casual