//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/strong/id.h"
#include "common/functional.h"
#include "common/algorithm.h"
#include "common/communication/log.h"


#include <vector>

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
               struct Set
               {
                  Set();

                  void add( strong::file::descriptor::id descriptor);
                  void remove( strong::file::descriptor::id descriptor);
                  
                  inline ::fd_set* native() { return &m_set;}

                  bool ready( strong::file::descriptor::id descriptor) const;

                  friend std::ostream& operator << ( std::ostream& out, const Set& value);

               private:
                  ::fd_set m_set;
               };
            } // directive
            struct Directive 
            {
               directive::Set read;
               // directive::Set write; no need for write as of now

               CASUAL_LOG_SERIALIZE(
               { 
                  CASUAL_SERIALIZE( read);
               })
            };

            namespace dispatch
            {
               namespace detail
               {
                  template< typename C> 
                  struct basic_reader 
                  {
                     basic_reader( strong::file::descriptor::id descriptor, C&& callback)
                        : m_descriptor( std::move( descriptor)), m_callback( std::move( callback)) {}

                     inline void read( strong::file::descriptor::id descriptor) { m_callback( descriptor);}
                     inline strong::file::descriptor::id descriptor() const { return m_descriptor;}

                  private:
                     strong::file::descriptor::id m_descriptor;
                     C m_callback;
                  };

                  Directive select( const Directive& directive);


                  namespace descriptor
                  {
                     template< typename H, typename F> 
                     auto dispatch( H& handler, F&& functor, traits::priority::tag< 0>) -> decltype( handler.descriptor(), void())
                     {
                        functor( handler, handler.descriptor());
                     }
                     
                     template< typename H, typename F> 
                     auto dispatch( H& handler, F&& functor, traits::priority::tag< 1>) -> decltype( handler.descriptors(), void())
                     {
                        for( auto descriptor : handler.descriptors())
                           functor( handler, descriptor);
                     }
                  } // descriptor
                  

                  namespace handle
                  {
                     template< typename H> 
                     void read( H& handler, strong::file::descriptor::id descriptor, traits::priority::tag< 0>) {}
                     
                     template< typename H> 
                     auto read( H& handler, strong::file::descriptor::id descriptor, traits::priority::tag< 1>)
                        -> decltype( handler.read( descriptor), void()) 
                     {
                        handler.read( descriptor);
                     }


                     template< typename H>
                     void dispatch( const Directive& directive, H& handler)
                     {
                        descriptor::dispatch( handler, [&directive]( auto& handler, auto descriptor)
                        {
                           if( directive.read.ready( descriptor))
                              handle::read( handler, descriptor, traits::priority::tag< 1>{});

                        }, traits::priority::tag< 1>{});
                     }
                  } // handle



                  // "sentinel"
                  template< typename F> 
                  void iterate( F&& functor) {}

                  template< typename F, typename H, typename... Hs> 
                  void iterate( F&& functor, H& handler, Hs&... handlers)
                  {
                     functor( handler);
                     iterate( std::forward< F>( functor), handlers...);
                  }


                  template< typename... Hs> 
                  void dispatch( const Directive& directive, Hs&... handlers)
                  {
                     auto invoke = [&directive]( auto& handler){
                        handle::dispatch( directive, handler);
                     };
                     iterate( invoke, handlers...);
                  }

                  namespace consume
                  {
                     template< typename H> 
                     auto handle( H& handler, traits::priority::tag< 0>) { return false;}

                     template< typename H>
                     auto handle( H& handler, traits::priority::tag< 1>) -> decltype( handler.consume())
                     {
                        return handler.consume();
                     }
                     
                     //! sentinel
                     constexpr bool dispatch() { return false;}

                     template< typename H, typename... Hs> 
                     bool dispatch( H& handler, Hs&... handlers)
                     {
                        return handle( handler, traits::priority::tag< 1>{}) || dispatch( handlers...);
                     }
                  } // consume

                  namespace handle
                  {
                     void error();
                  } // handle

               } // detail

               namespace create
               {
                  template< typename C>
                  auto reader( strong::file::descriptor::id descriptor, C&& callback)
                  {
                     return detail::basic_reader< std::decay_t< C>>{ descriptor, std::forward< C>( callback)};
                  }
               } // create

               template< typename... Ts>  
               void pump( const Directive& directive, Ts&&... handlers)
               {
                  while( true)
                  {
                     try
                     {
                        // make sure we try to consume from the handlers before
                        // we might block forever. Handlers could have cached messages
                        // that wont be triggered via multiplexing on file descriptors
                        while( detail::consume::dispatch( handlers...))
                           ; // no-op.

                        auto result = detail::select( directive);
                        detail::dispatch( result, handlers...);
                     }
                     catch( ...)
                     {
                        detail::handle::error();
                     }
                  }
               }

               namespace conditional
               {
                  //! same as dispatch::pump, but calls `done` to ask caller if we're done or not
                  template< typename... Ts>  
                  void pump( const Directive& directive, const std::function< bool()>& done, Ts&&... handlers)
                  {
                     while( true)
                     {
                        try
                        {
                           // make sure we try to consume from the handlers before
                           // we might block forever. Handlers could have cached messages
                           // that wont be triggered via multiplexing on file descriptors
                           while( detail::consume::dispatch( handlers...))
                              ; // no-op.

                           if( done())
                              return;

                           auto result = detail::select( directive);
                           detail::dispatch( result, handlers...);
                        }
                        catch( ...)
                        {
                           detail::handle::error();
                        }
                     }
                  }
                  
               } // conditional
            } // dispatch

            namespace block
            {
               //! block until descriptor is ready for read.
               void read( strong::file::descriptor::id descriptor);

            } // block
         } // select
      } // communication
   } // common
} // casual