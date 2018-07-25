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

               private:
                  ::fd_set m_set;
               };
            } // directive
            struct Directive 
            {
               directive::Set read;
            };

            namespace dispatch
            {
               namespace detail
               {
                  template< typename D, typename C> 
                  struct basic_reader 
                  {
                     using descriptors_type = D;

                     basic_reader( descriptors_type descriptor, C&& callback)
                        : m_descriptor( std::move( descriptor)), m_callback( std::move( callback)) {}

                     inline void read( strong::file::descriptor::id descriptor) { m_callback( descriptor);}
                     inline descriptors_type descriptors() const { return m_descriptor;}

                  private:
                     descriptors_type m_descriptor;
                     C m_callback;
                  };

                  Directive select( const Directive& directive);


                  template< typename H> 
                  void reader_dispatch( const Directive& directive, H& handler, strong::file::descriptor::id descriptor)
                  {
                     if( directive.read.ready( descriptor))
                     {
                        handler.read( descriptor);
                     }
                  }

                  template< typename H> 
                  void reader_dispatch( const Directive& directive, H& handler, const std::vector< strong::file::descriptor::id>& descriptors)
                  {
                     for( auto descriptor : descriptors)
                     {
                        reader_dispatch( directive, handler, descriptor);
                     }
                  }

                  // "sentinel"
                  inline void dispatch( const Directive& directive) {}

                  template< typename H, typename... Hs> 
                  void dispatch( const Directive& directive, H& handler, Hs&... holders)
                  {
                     reader_dispatch( directive, handler, handler.descriptors());
                     dispatch( directive, holders...);
                  }
               } // detail

               namespace create
               {
                  template< typename D, typename C>
                  auto reader( D&& descriptors, C&& callback)
                  {
                     return detail::basic_reader< std::decay_t< D>, std::decay_t< C>>{ std::forward< D>( descriptors), std::forward< C>( callback)};
                  }
               } // create

               template< typename... Ts>  
               void pump( const Directive& directive, Ts&&... holders)
               {
                  while( true)
                  {
                     auto result = detail::select( directive);
                     detail::dispatch( result, holders...);
                  }
               }
            } // dispatch

         } // select
      } // communication
   } // common
} // casual