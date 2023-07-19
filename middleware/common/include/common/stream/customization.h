//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

namespace casual
{
   //! to enable customization points for stream/logging
   namespace common::stream::customization
   {
      // a "template type deduction delay" type for write" - usage later: stream::customization::delay< some_tag>::write( ...)
      template< typename T>
      struct delay;

      //! kicks in if T does not have an ostream stream operator
      template< typename T, typename Enable = void>
      struct point;

      namespace supersede
      {
         // highest priority, and will supersede ostream stream operator
         // used to 'override' standard defined stream operators
         template< typename T, typename Enable = void>
         struct point;
      } // supersede


      namespace detail
      {
         namespace dispatch
         {
            // highest priority, takes all that have specialized customization::supersede::point
            template< typename T> 
            auto write( std::ostream& out, T&& value, traits::priority::tag< 2>) 
               -> decltype( void( stream::customization::supersede::point< std::remove_cvref_t< T>>::stream( out, std::forward< T>( value))), out)
            {
               stream::customization::supersede::point< std::remove_cvref_t< T>>::stream( out, std::forward< T>( value));
               return out;
            }

            // takes all that can use the ostream stream operator, including the one defined above.
            template< typename T> 
            auto write( std::ostream& out, T&& value, traits::priority::tag< 1>) 
               -> decltype( out << std::forward< T>( value))
            {
               return out << std::forward< T>( value);
            }

            template< typename T> 
            auto write( std::ostream& out, T&& value, traits::priority::tag< 0>) 
               -> decltype( void( stream::customization::point< std::remove_cvref_t< T>>::stream( out, std::forward< T>( value))), out)
            {
               stream::customization::point< std::remove_cvref_t< T>>::stream( out, std::forward< T>( value));
               return out;
            }
            
         } // dispatch
         
         //! internal priority dispatch to get the right customization, including ostream stream operator
         template< typename T> 
         auto write( std::ostream& out, const T& value)
            -> decltype( dispatch::write( out, value, traits::priority::tag< 2>{}))
         {
            return dispatch::write( out, value, traits::priority::tag< 2>{});
         }
         
      } // detail

   } // common::stream::customization
} // casual