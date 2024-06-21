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
   namespace common::serialize::writer
   {
      namespace detail
      {
         template< typename P, typename T>
         concept has_consume = requires( P& protocol, T& value)
         {
            protocol.consume( value);
         };

         template< typename P, typename T>
         concept has_consume_to = requires( P& protocol)
         {
            { protocol.consume()} -> std::same_as< T>;
         };

         namespace convert
         {
            namespace detail
            {

               template< typename S, typename D>
               requires std::convertible_to< S, D>
               void value( S&& source, D& destination, common::traits::priority::tag< 4>)
               {
                  destination = std::forward< S>( source);
               }

               template< concepts::string::like S>
               void value( S&& source, platform::binary::type& destination, common::traits::priority::tag< 3>)
               {
                  algorithm::copy( view::binary::make( source), destination);
               }

               template< concepts::binary::like S>
               void value( S&& source, std::string& destination, common::traits::priority::tag< 2>)
               {
                  destination.resize( std::size( source));
                  algorithm::copy( source, view::binary::make( destination));
               }

               template< concepts::string::like S>
               void value( S&& source, std::string& destination, common::traits::priority::tag< 1>)
               {
                  destination.assign( std::begin( source), std::end( source));
               }

               template< concepts::binary::like S>
               void value( S&& source, std::ostream& destination, common::traits::priority::tag< 0>)
               {
                  auto span = view::binary::to_string_like( source);
                  destination.write( span.data(), span.size());
               }

               template< concepts::string::like S>
               void value( S&& source, std::ostream& destination, common::traits::priority::tag< 0>)
               {
                  destination.write( source.data(), source.size());
               }
               
            } // detail
            
            template< typename S, typename D>
            auto value( S&& source, D& destination)
               -> decltype( detail::value( std::forward< S>( source), destination, common::traits::priority::tag< 4>{}))
            {
               detail::value( std::forward< S>( source), destination, common::traits::priority::tag< 4>{});
            }
         } // convert

         //! highest - native handling for the destination
         template< typename P, typename D>
         requires has_consume_to< P, D>
         void consume( P& protocol, D& destination, common::traits::priority::tag< 6>) 
         {
            destination = protocol.consume();
         }

         //! highest - native handling for the destination
         template< typename P, typename D>
         requires detail::has_consume< P, D>
         void consume( P& protocol, D& destination, common::traits::priority::tag< 5>) 
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
         requires detail::has_consume< P, platform::binary::type>
         void consume( P& protocol, D& destination, common::traits::priority::tag< 3>) 
         {
            platform::binary::type value;
            protocol.consume( value);
            convert::value( std::move( value), destination);
         }

         template< typename P, typename D>
         requires detail::has_consume< P, std::string>
         void consume( P& protocol, D& destination, common::traits::priority::tag< 2>) 
         {
            std::string value;
            protocol.consume( value);
            convert::value( std::move( value), destination);
         }

         template< typename P, typename D>
         requires detail::has_consume< P, std::ostream>
         auto consume( P& protocol, D& destination, common::traits::priority::tag< 1>) 
         {
            std::ostringstream stream;
            protocol.consume( stream);
            convert::value( std::move( stream).str(), destination);
         }
         
         //! lowest - no consume
         //! this is for the rare 'adaptors' that act like an archive/protocol 
         //template< typename P, typename D>
         //void consume( P& protocol, D& destination, common::traits::priority::tag< 0>)
         //{
         //}
         
      } // detail

      //! dispatch to choose the best _consume_ alternative for the protocol
      //! and the destination. 
      //! It's a combinatorial explosion problem, and priority tag and expression SFINAE 
      //! seems to be the easiest way to solve it.
      template< typename P, typename D>
      void consume( P& protocol, D& destination)
      {
         // we do not consume if the protocol explicitly says so.
         if constexpr( ! archive::has::property< P, archive::Property::no_consume>)
            detail::consume( protocol, destination, common::traits::priority::tag< 6>{});
      }
      
   } // common::serialize::writer
} // casual
