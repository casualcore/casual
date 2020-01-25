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

               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
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

                  namespace has
                  {
                     namespace detail
                     {
                        template< typename T>
                        using read = decltype( std::declval< T&>().read( strong::file::descriptor::id{}));

                        template< typename T>
                        using descriptor = decltype( std::declval< T&>().descriptor()); 

                        template< typename T>
                        using descriptors = decltype( std::declval< T&>().descriptors());

                        template< typename T>
                        using consume = decltype( std::declval< T&>().consume());                   
                     } // detail

                     template< typename T>
                     using read = traits::detect::is_detected< detail::read, T>;

                     template< typename T>
                     using descriptor = traits::detect::is_detected< detail::descriptor, T>;

                     template< typename T>
                     using descriptors = traits::detect::is_detected< detail::descriptors, T>;

                     template< typename T>
                     using consume = traits::detect::is_detected< detail::consume, T>;
                  } // has

                  namespace descriptor
                  {
                     template< typename H, typename F> 
                     std::enable_if_t< has::descriptor< H>::value>
                     dispatch( H& handler, F&& functor)
                     {
                        functor( handler, handler.descriptor());
                     }
                     
                     template< typename H, typename F> 
                     std::enable_if_t< has::descriptors< H>::value>
                     dispatch( H& handler, F&& functor)
                     {
                        for( auto descriptor : handler.descriptors())
                           functor( handler, descriptor);
                     }
                  } // descriptor
                  

                  namespace read
                  {
                     template< typename H> 
                     std::enable_if_t< ! has::read< H>::value>
                     dispatch( const Directive& directive, H& handler) {}

                     template< typename H>
                     std::enable_if_t< has::read< H>::value>
                     dispatch( const Directive& directive, H& handler)
                     {
                        descriptor::dispatch( handler, [&directive]( auto& handler, auto descriptor)
                        {
                           if( directive.read.ready( descriptor))
                           {
                              handler.read( descriptor);
                           }
                        });
                     }
                  } // read



                  // "sentinel"
                  template< typename D> 
                  void iterate( D&& dispatch) {}

                  template< typename F, typename H, typename... Hs> 
                  void iterate( F&& functor, H& handler, Hs&... holders)
                  {
                     functor( handler);
                     iterate( std::forward< F>( functor), holders...);
                  }


                  template< typename... Hs> 
                  void dispatch( const Directive& directive, Hs&... holders)
                  {
                     auto read = [&directive]( auto& handler){
                        read::dispatch( directive, handler);
                     };
                     iterate( read, holders...);
                  }

                  namespace consume
                  {
                     template< typename H> 
                     std::enable_if_t< ! has::consume< H>::value, bool>
                     handle( H& handler) { return false;}

                     template< typename H>
                     std::enable_if_t< has::consume< H>::value, bool>
                     handle( H& handler)
                     {
                        return handler.consume();
                     }

                     constexpr bool dispatch() { return false;}

                     template< typename H, typename... Hs> 
                     bool dispatch( H& handler, Hs&... handlers)
                     {
                        return handle( handler) || dispatch( handlers...);
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