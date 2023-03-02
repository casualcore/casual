//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"
#include "common/algorithm.h"

#include <sstream>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace writer
         {
            namespace detail
            {
               namespace convert
               {
                  namespace detail
                  {
                     template< typename S, typename D>
                     auto value( S&& source, D& destination, common::traits::priority::tag< 2>)
                        -> decltype( destination = std::forward< S>( source), void())
                     {
                        destination = std::forward< S>( source);
                     }

                     template< typename S, typename D>
                     auto value( S&& source, D& destination, common::traits::priority::tag< 1>)
                        -> decltype( common::algorithm::copy( source, destination), void())
                     {
                        common::algorithm::copy( source, destination);
                     }

                     template< typename S, typename D>
                     auto value( S&& source, D& destination, common::traits::priority::tag< 0>)
                        -> decltype( destination.write( source.data(), source.size()), void())
                     {
                        destination.write( source.data(), source.size());
                     }
                     
                  } // detail
                  
                  template< typename S, typename D>
                  auto value( S&& source, D& destination)
                     -> decltype( detail::value( std::forward< S>( source), destination, common::traits::priority::tag< 2>{}), void())
                  {
                     detail::value( std::forward< S>( source), destination, common::traits::priority::tag< 2>{});
                  }
               } // convert

               //! highest - native handling for the destination
               template< typename P, typename D>
               auto consume( P& protocol, D& destination, common::traits::priority::tag< 6>) 
                  -> decltype( destination = protocol.consume(), void())
               {
                  destination = protocol.consume();
               }

               //! highest - native handling for the destination
               template< typename P, typename D>
               auto consume( P& protocol, D& destination, common::traits::priority::tag< 5>) 
                  -> decltype( protocol.consume( destination), void())
               {
                  protocol.consume( destination);
               }


               template< typename P, typename D>
               auto consume( P& protocol, D& destination, common::traits::priority::tag< 4>) 
                  -> decltype( convert::value( protocol.consume(), destination))
               {
                  convert::value( protocol.consume(), destination);
               }

               template< typename P, typename D>
               auto consume( P& protocol, D& destination, common::traits::priority::tag< 3>) 
                  -> decltype( protocol.consume( std::declval< platform::binary::type&>()), void())
               {
                  platform::binary::type value;
                  protocol.consume( value);
                  convert::value( std::move( value), destination);
               }

               template< typename P, typename D>
               auto consume( P& protocol, D& destination, common::traits::priority::tag< 2>) 
                  -> decltype( protocol.consume( std::declval< std::string&>()), void())
               {
                  std::string value;
                  protocol.consume( value);
                  convert::value( std::move( value), destination);
               }

               template< typename P, typename D>
               auto consume( P& protocol, D& destination, common::traits::priority::tag< 1>) 
                  -> decltype( protocol.consume( std::declval< std::ostream&>()), void())
               {
                  std::ostringstream stream;
                  protocol.consume( stream);
                  convert::value( std::move( stream).str(), destination);
               }
               
               //! lowest - no consume
               //! this is for the rare 'adaptors' that act like an archive/protocol 
               template< typename P, typename D>
               void consume( P& protocol, D& destination, common::traits::priority::tag< 0>)
               {
               }
               
            } // detail

            //! dispatch to choose the best _consume_ alternative for the protocol
            //! and the destination. 
            //! It's a combinatorical problem, and priority tag and expression SFINAE 
            //! seems to be the easiest way to solve it.
            template< typename P, typename D>
            void consume( P& protocol, D& destination)
            {
               detail::consume( protocol, destination, common::traits::priority::tag< 6>{});
            }
            
         } // writer
      } // serialize
   } // common
} // casual