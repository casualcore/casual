//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <utility>

namespace casual 
{
   namespace common::predicate
   {
      //! @returns a composite predicate of all `predicates`, when invoked
      //!    return _logical AND_ for all `predicates`
      template< typename... Ps>
      [[nodiscard]] auto conjunction( Ps&&... predicates)
      {
         return [=]( auto&&... param)
         {
            return ( ... && predicates( param...));
         };
      }

      //! @returns a composite predicate of all `predicates`, when invoked
      //!    return _logical OR_ for all `predicates`
      template< typename... Ps>
      [[nodiscard]] auto disjunction( Ps&&... predicates)
      {
         return [=]( auto&&... param)
         {
            return ( ... || predicates( param...));
         };
      }

      template< typename P>
      [[nodiscard]] auto composition( P&& predicate) { return std::forward< P>( predicate);}

      template< typename P, typename... Ts>
      [[nodiscard]] auto composition( P&& predicate, Ts&&... ts)
      {
         return [=]( auto&& value)
         {
            return predicate( composition( std::move( ts)...)( std::forward< decltype( value)>( value)));
         };
      }

      //! reverse the call order of left and right to a predicate
      template< typename T>
      [[nodiscard]] auto inverse( T&& functor)
      {
         return [functor]( auto&& l, auto&& r){ return functor( r, l);};
      }

      //! negates a predicate
      template< typename T>
      [[nodiscard]] auto negate( T&& functor)
      {
         return [functor]( auto&&... param){ return ! functor( param...);};
      }

      template< typename T>
      [[nodiscard]] auto boolean( T&& value) -> decltype( static_cast< bool>( value))
      {
         return static_cast< bool>( value);
      }

      [[nodiscard]] inline auto boolean()
      {
         return []( auto&& value){ return boolean( value);};
      }

      namespace value
      {
         template< typename T>
         [[nodiscard]] auto equal( T&& wanted)
         {
            return [ wanted = std::forward< T>( wanted)]( auto&& value){ return value == wanted;};
         }
      } // value

      namespace adapter
      {
         //! wraps and invoke `predicate` with `value.second`
         //! useful for iterate over map-like containers. 
         template< typename P>
         [[nodiscard]] auto second( P&& predicate)
         {
            return [ predicate = std::forward< P>( predicate)]( auto& pair)
            {
               return predicate( pair.second); 
            };
         }


         [[nodiscard]] inline auto first()
         {
            return []( auto& pair) -> decltype( pair.first)
            {
               return pair.first;
            };
         }

         [[nodiscard]] inline auto second()
         {
            return []( auto& pair) -> decltype( pair.second)
            {
               return pair.second;
            };
         }
      } // adapter


   } // common::predicate
} // casual 