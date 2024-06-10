//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/flag/enum.h"

#include "common/algorithm.h"
#include "common/stream/customization.h"

#include <bitset>

namespace casual
{
   namespace common::flag
   {
      constexpr bool empty( casual::common::flag::enum_flag_like auto flags)
      {
         return std::to_underlying( flags) == 0;
      }

      //! @returns true if `flags` contains `flag`
      //! @pre `flag` != 0
      template< casual::common::flag::enum_flag_like T>
      constexpr bool contains( T flags, T flag)
      {
         return ! flag::empty( flag) && ( ( flags & flag) == flag);
      }
      
      
      //! @returns the number of bits/flags in `flags` set to `1`
      template< casual::common::flag::enum_flag_like T>
      constexpr auto count( T flags)
      {
         using type = std::underlying_type_t< T>;

         if constexpr( std::is_unsigned_v< type>)
            return std::popcount( std::to_underlying( flags));
         else
            return std::popcount( static_cast< std::make_unsigned_t< type>>( std::to_underlying( flags)));
      }

      //! @returns true if there are no bits set in `flag` outside the `expected` bits; 
      template< casual::common::flag::enum_flag_like T>
      constexpr bool valid( T expected, T flag)
      {
         return ( ~std::to_underlying( expected) & std::to_underlying( flag)) == 0;
      }

      //! @returns a converted flag type from `other` to the same type as `filter` that only has bits sets that
      //! are set in `filter`
      template< casual::common::flag::enum_flag_like T, typename U>
      constexpr auto convert( T filter, U other) -> decltype( static_cast< T>( other))
      {
         return filter & static_cast< T>( other);
      }
   } // common::flag
   
   namespace common::stream::customization::supersede
   {
      template< common::flag::enum_flag_like T> 
      struct point< T>
      {  
         //! tries to print the flag in the best representation possible
         static void stream( std::ostream& out, T value)
         {
            // if we can use _description( enum)_ we use it.
            if constexpr( common::flag::detail::can_use_description< T>)
            {
               out << '[';

               auto flag = common::flag::detail::description_type( value);
               using flag_type = decltype( flag);

               using underlying_type = std::underlying_type_t< flag_type>;
               using unsigned_type = std::make_unsigned_t< underlying_type>;
               constexpr auto bit_count = std::numeric_limits< unsigned_type>::digits;
               std::bitset< bit_count> bits( std::to_underlying( flag));

               algorithm::for_n< bit_count>( [ &out, bits]( unsigned_type index) mutable
               {
                  if( ! bits.test( index))
                     return;

                  // print the flag
                  out << ' ' << description( static_cast< flag_type>( unsigned_type( 1) << index));

                  // "consume" the bit/flag
                  bits.reset( index);

                  // prepare next if we've got any left
                  if( bits.any())
                     out << ",";

                  // we don't get early-return, we always go through all bits...
               });

               out << ']';
            }
            else
            {
               out << "0x" << std::hex << std::to_underlying( value) << std::dec;
            }
         }
      };
      
   } // common::stream::customization::supersede

} // casual


