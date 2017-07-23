//!
//! casual
//!

#ifndef CASUAL_COMMON_FLAG_H_
#define CASUAL_COMMON_FLAG_H_

#include "common/marshal/marshal.h"
#include "common/exception/system.h"
#include "common/string.h"
#include "common/traits.h"

#include <initializer_list>
#include <type_traits>
//#include <cstdlib>


namespace casual
{
	namespace common
	{

		template< std::uint64_t flags, typename T>
		constexpr inline bool flag( T value)
		{
			return ( value & flags) == flags;
		}


      template< typename E>
      struct Flags
      {
         using enum_type = E;
         static_assert( std::is_enum< enum_type>::value, "E has to be of enum type");

         using underlaying_type = typename std::underlying_type< enum_type>::type;

         constexpr Flags() = default;

         template< typename... Enums>
         constexpr Flags( enum_type e, Enums... enums) : Flags( bitmask( e, enums...)) {}



         constexpr Flags convert( underlaying_type flags) const
         {
            if( flags & ~underlaying())
            {
               throw exception::system::invalid::Argument{ string::compose( "invalid flags: ", flags, " limit: ", *this)};
            }
            return { flags};
         }

         template< typename E2>
         constexpr Flags convert( Flags< E2> flags) const
         {
            return convert( flags.underlaying());
         }

         constexpr bool empty() const noexcept { return m_flags == underlaying_type{};}

         constexpr explicit operator bool() const noexcept { return ! empty();}

         constexpr bool exist( enum_type flag) const
         {
            return ( m_flags & underlaying( flag)) == underlaying( flag);
         };


         constexpr underlaying_type underlaying() const noexcept { return m_flags;}


         constexpr friend Flags& operator |= ( Flags& lhs, Flags rhs) { lhs.m_flags |= rhs.m_flags; return lhs;}
         constexpr friend Flags operator | ( Flags lhs, Flags rhs) { return Flags{ lhs.m_flags | rhs.m_flags};}

         constexpr friend Flags& operator &= ( Flags& lhs, Flags rhs) { lhs.m_flags &= rhs.m_flags; return lhs;}
         constexpr friend Flags operator & ( Flags lhs, Flags rhs) { return Flags{ lhs.m_flags & rhs.m_flags};}

         constexpr friend Flags& operator ^= ( Flags& lhs, Flags rhs) { lhs.m_flags ^= rhs.m_flags; return lhs;}
         constexpr friend Flags operator ^ ( Flags lhs, Flags rhs) { return Flags{ lhs.m_flags ^ rhs.m_flags};}

         constexpr friend Flags operator ~ ( Flags lhs) { return Flags{ ~lhs.m_flags};}

         constexpr friend bool operator == ( Flags lhs, Flags rhs) { return lhs.m_flags == rhs.m_flags;}
         constexpr friend bool operator != ( Flags lhs, Flags rhs) { return ! ( lhs == rhs);}

         constexpr friend Flags operator - ( Flags lhs, Flags rhs) { return Flags{ lhs.m_flags & ~rhs.m_flags};}
         constexpr friend Flags& operator -= ( Flags& lhs, Flags rhs) { lhs.m_flags &= ~rhs.m_flags; return lhs;}


         friend std::ostream& operator << ( std::ostream& out, Flags flags)
         {
            return out << "0x" << std::hex << flags.m_flags << std::dec;
         }

         CASUAL_CONST_CORRECT_MARSHAL(
            archive & m_flags;
         )


      private:

         constexpr Flags( underlaying_type bitmask) : m_flags( bitmask) {}

         static constexpr underlaying_type bitmask( enum_type e) { return underlaying( e);}

         template< typename... Enums>
         static constexpr underlaying_type bitmask( enum_type e, Enums... enums)
         {
            static_assert( traits::is_same< enum_type, Enums...>::value, "wrong enum type");

            return underlaying( e) | bitmask( enums...);
         }

         static constexpr underlaying_type underlaying( enum_type e) { return static_cast< underlaying_type>( e);}


         underlaying_type m_flags = underlaying_type{};
      };

	} // common
} // casual


#endif /* CASUAL_UTILITY_FLAG_H_ */
