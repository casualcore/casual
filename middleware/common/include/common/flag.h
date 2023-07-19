//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"
#include "common/string.h"
#include "common/traits.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <initializer_list>
#include <type_traits>

namespace casual
{
   namespace common
   {

      namespace has
      {
         template< auto flags, typename T>
         constexpr inline bool flag( T value)
         {
            return ( value & flags) == flags;
         }
      } // has


      template< typename E>
      struct Flags
      {
         using enum_type = E;
         using underlying_type = std::underlying_type_t< enum_type>;

         static_assert( std::is_enum< enum_type>::value, "E has to be of enum type");
         

         //! just a helper to easier deduce the enum-type
         constexpr static auto type() noexcept { return enum_type{};}         

         constexpr Flags() = default;

         template< typename... Enums>
         constexpr Flags( enum_type e, Enums... enums) noexcept : Flags( bitmask( e, enums...)) {}
 
         constexpr explicit Flags( underlying_type flags) noexcept : m_flags( flags) {}

         constexpr Flags convert( underlying_type flags) const
         {
            if( flags & ~underlying())
               code::raise::error( code::casual::invalid_argument, "flags: ", flags, " limit: ", *this);

            return Flags{ flags};
         }

         template< typename E2>
         constexpr Flags convert( Flags< E2> flags) const
         {
            return convert( flags.underlying());
         }

         constexpr bool empty() const noexcept { return m_flags == underlying_type{};}

         constexpr explicit operator bool() const noexcept { return ! empty();}

         constexpr bool exist( enum_type flag) const noexcept
         {
            return ( m_flags & std::to_underlying( flag)) == std::to_underlying( flag);
         }

         platform::size::type bits() const noexcept
         {
            return std::bitset< std::numeric_limits< underlying_type>::digits>{ static_cast< unsigned long long>( m_flags)}.count();
         }

         constexpr underlying_type underlying() const noexcept { return m_flags;}


         constexpr friend Flags& operator |= ( Flags& lhs, Flags rhs) noexcept { lhs.m_flags |= rhs.m_flags; return lhs;}
         constexpr friend Flags operator | ( Flags lhs, Flags rhs) noexcept { return Flags( lhs.m_flags | rhs.m_flags);}
         
         constexpr friend Flags& operator &= ( Flags& lhs, Flags rhs) noexcept { lhs.m_flags &= rhs.m_flags; return lhs;}
         constexpr friend Flags operator & ( Flags lhs, Flags rhs) noexcept { return Flags( lhs.m_flags & rhs.m_flags);}

         constexpr friend Flags& operator ^= ( Flags& lhs, Flags rhs) noexcept { lhs.m_flags ^= rhs.m_flags; return lhs;}
         constexpr friend Flags operator ^ ( Flags lhs, Flags rhs) noexcept { return Flags( lhs.m_flags ^ rhs.m_flags);}

         constexpr friend Flags operator ~ ( Flags lhs) noexcept { return Flags{ ~lhs.m_flags};}

         constexpr friend bool operator == ( Flags lhs, Flags rhs) noexcept { return lhs.m_flags == rhs.m_flags;}
         constexpr friend bool operator != ( Flags lhs, Flags rhs) noexcept { return ! ( lhs == rhs);}

         //! More _easy to reason about_  operators that does what one thinks they do.
         //! @{
         constexpr friend Flags operator - ( Flags lhs, Flags rhs) noexcept { return Flags{ lhs.m_flags & ~rhs.m_flags};}
         constexpr friend Flags& operator -= ( Flags& lhs, Flags rhs) noexcept { lhs.m_flags &= ~rhs.m_flags; return lhs;}

         constexpr friend Flags operator + ( Flags lhs, Flags rhs) noexcept { return lhs | rhs;}
         constexpr friend Flags& operator += ( Flags lhs, Flags rhs) noexcept { return lhs |= rhs;}
         //! @}


         constexpr friend std::ostream& operator << ( std::ostream& out, Flags flags)
         {
            return print( out, flags, traits::priority::tag< 1>{});
         }

         CASUAL_FORWARD_SERIALIZE( m_flags)

      private:

         template< typename Enum>
         static auto print( std::ostream& out, Flags< Enum> flags, traits::priority::tag< 1>) -> decltype( out << description( Enum{}))
         {
            // TODO maintainence: This can probably be done a lot better...
            if( ! flags)
               return out << "[]";

            out << "[ ";

            using unsigned_type = std::make_unsigned_t< underlying_type>;
            constexpr auto bits = std::numeric_limits< unsigned_type>::digits;
   
            algorithm::for_n< bits>( [ &out, flags]( unsigned_type index)
            {
               const unsigned_type flag = unsigned_type( 1) << index;
               if( ( flags.underlying() & flag) == 0)
                  return;
               
               out << description( Enum( flag));

               const auto next = index + 1;

               if( next == bits)
                  return;

               constexpr unsigned_type filled = std::numeric_limits< unsigned_type>::max();
               const unsigned_type mask( filled << next);
               if( mask & static_cast< unsigned_type>( flags.underlying()))
                  out << ", ";
            });
            return out << ']';
         }

         template< typename Enum>
         static std::ostream& print( std::ostream& out, Flags< Enum> flags, traits::priority::tag< 0>)
         {
            return out << "0x" << std::hex << flags.underlying() << std::dec;
         }


         template< typename... Enums>
         static constexpr underlying_type bitmask( Enums... enums) noexcept
         {
            static_assert( concepts::same_as< enum_type, Enums...>, "wrong enum type");

            return ( 0 | ... | std::to_underlying( enums) );
         }

         underlying_type m_flags = underlying_type{};
      };

      namespace flags
      {
         template< typename Enum, typename... Enums>
         constexpr auto compose( Enum flag, Enums... flags) requires concepts::same_as< Enum, Enums...>
         {
            return Flags< Enum>{ flag, flags...};
         }
      } // flags

   } // common
} // casual



