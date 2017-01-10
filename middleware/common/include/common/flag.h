//!
//! causual
//!

#ifndef CASUAL_COMMON_FLAG_H_
#define CASUAL_COMMON_FLAG_H_

#include <cstdlib>

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

         Flags() = default;
         Flags( enum_type e) : m_flags( static_cast< underlaying_type>( e)) {}

         Flags( std::initializer_list< enum_type> enums)
         {
            for( auto e : enums) { binary_or( e);}
         }

         explicit operator bool() const noexcept
         {
            return m_flags != underlaying_type{};
         }

         underlaying_type underlaying() const noexcept { return m_flags;}


         friend Flags& operator |= ( Flags& lhs, enum_type rhs) { lhs.binary_or( rhs); return lhs;}
         friend Flags operator | ( Flags lhs, enum_type rhs) { lhs.binary_or( rhs); return lhs;}
         friend Flags operator | ( enum_type lhs, Flags rhs) { rhs.binary_or( rhs); return rhs;}

         friend Flags& operator &= ( Flags& lhs, enum_type rhs) { lhs.binary_and( rhs); return lhs;}
         friend Flags operator & ( Flags lhs, enum_type rhs) { lhs.binary_and( rhs); return lhs;}
         friend Flags operator & ( enum_type lhs, Flags rhs) { rhs.binary_and( rhs); return rhs;}


         friend bool operator == ( Flags lhs, Flags rhs) { return lhs.m_flags == rhs.m_flags;}
         friend bool operator == ( Flags lhs, enum_type rhs) { return lhs.m_flags == static_cast< underlaying_type>( rhs);}
         friend bool operator == ( enum_type lhs, Flags rhs) { return rhs == lhs;}

         friend std::ostream& operator << ( std::ostream& out, Flags flags)
         {
            return out << std::ios::hex << flags.m_flags;
         }


      private:

         void binary_or( enum_type e)
         {
            m_flags |= static_cast< underlaying_type>( e);
         }

         void binary_and( enum_type e)
         {
            m_flags &= static_cast< underlaying_type>( e);
         }


         underlaying_type m_flags = underlaying_type{};
      };


	} // comon
} // casual


#endif /* CASUAL_UTILITY_FLAG_H_ */
