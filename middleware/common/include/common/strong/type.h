//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"
#include "common/compare.h"
#include "common/serialize/macro.h"
#include "common/uuid.h"

#include <iosfwd>

namespace casual
{
   namespace common::strong
   {
      namespace detail
      {
         //! some helper traits to enable/disable behavior
         //! @{
         namespace has
         {
            template< typename T>
            concept bool_operator = requires( T a)
            {
               static_cast< bool>( a);
            };

            template< typename T>
            concept initialize = requires
            {
               T::initialize();
            };

            template< typename T>
            concept generate = requires
            {
               T::generate();
            };

            template< typename P, typename T>
            concept static_valid = requires( T a)
            {
               { P::valid( a) } -> std::convertible_to< bool>;
            };

            template< typename T>
            concept valid = requires( T a)
            {
               { a.valid()} -> std::convertible_to< bool>;
            };

            template< typename T>
            concept value_hash = requires( const T& a)
            {
               a.value();
               { std::hash< std::remove_cvref_t< decltype( a.value())>>{}( a.value())}; 
            };

            template< typename T, typename P, typename U>
            concept extended_equality = ! std::same_as< T, U> && requires( const T& a, const U& b) 
            {
               { a == b} -> std::convertible_to< bool>;
               typename P::extended_equality;
            };

         } // has

         //! @}
              
      } // detail


      //! "id" abstraction to help make "id-handling" more explicit, typesafe and
      //! enable overloading on a specific id-type
      //! `Policy` can be 'empty' and just act as a `tag` to make the type unique
      template< typename T, typename Policy>
      struct Type 
      {
         // indicate that we can use this type in "environment context"
         using environment_variable_enable = void;

         using value_type = T;
         using policy_type = Policy;
         using value_traits = common::traits::type< value_type>;

         constexpr Type() noexcept = default;

         template< std::convertible_to< T> V> 
         constexpr explicit Type( V&& value) noexcept( value_traits::nothrow_move_construct)
            : m_value( std::forward< V>( value)) {}

         //! creates a Type by emplacing on the internal value_type.
         template< typename... Ts>
         constexpr static auto emplace( Ts&&... ts)
         {
            return Type{ value_type{ std::forward< Ts>( ts)...}};
         }

         //! return id by value if suitable. const ref otherwise.
         constexpr auto value() const noexcept -> common::traits::by_value_or_const_ref_t< T>
         { 
            return m_value;
         }

         //! @returns true if the underlying value is valid
         constexpr explicit operator bool() const noexcept
            requires detail::has::bool_operator< value_type> || detail::has::static_valid< policy_type, value_type>
         {
            if constexpr( detail::has::static_valid< policy_type, value_type>)
               return policy_type::valid( m_value);

            return static_cast< bool>( m_value);
         }

         //! @returns true if the underlying value is valid
         constexpr bool valid() const noexcept 
            requires detail::has::bool_operator< value_type> || detail::has::static_valid< policy_type, value_type>
         {
            if constexpr( detail::has::static_valid< policy_type, value_type>)
               return policy_type::valid( m_value);

            return static_cast< bool>( m_value);
         }

         //! enabled if the policy_type has a `generate()` static function defined.
         //! @returns `Type` that has a value generated by policy_type::generate()
         static Type generate() noexcept requires detail::has::generate< policy_type>
         {
            return Type{ policy_type::generate()};
         }
         
         // forward serialization
         CASUAL_FORWARD_SERIALIZE( m_value)

         //! accessor for the underlying type by ref. Should only
         //! be used when interacting with 'C-api:s'
         //! @{
         value_type& underlying() noexcept { return m_value;}
         const value_type& underlying() const noexcept { return m_value;}
         //! @}

         friend auto operator <=> ( const Type& lhs, const Type& rhs) noexcept requires std::three_way_comparable< value_type> { return lhs.m_value <=> rhs.m_value;};
         friend bool operator == ( const Type& lhs, const Type& rhs) noexcept requires std::equality_comparable< value_type> { return lhs.m_value == rhs.m_value;}
         friend bool operator < ( const Type& lhs, const Type& rhs) noexcept requires concepts::compare::less< value_type> { return lhs.m_value < rhs.m_value;}

      protected:
      
         constexpr static value_type initialize() noexcept
         {
            if constexpr( detail::has::initialize< policy_type>)
               return policy_type::initialize();
            return value_type{};
         }

         value_type m_value = initialize();
      };


      //! If the underlying type has equality operations defined for other types we expose
      //! these if the policy type has defined extended_equality
      template< typename T, typename Policy, typename V>
      inline bool operator == ( const Type< T, Policy>& lhs, const V& rhs) requires detail::has::extended_equality< T, Policy, V> 
      { 
         return lhs.value() == rhs;
      }

      namespace detail::ostream
      {
         template< typename T>
         auto stream( std::ostream& out, const T& value, common::traits::priority::tag< 1>) 
            -> decltype( void( T::policy_type::stream( out, value.value())), out)
         {
            T::policy_type::stream( out, value.value());
            return out;
         }

         template< typename T>
         auto stream( std::ostream& out, const T& value, common::traits::priority::tag< 0>) 
            -> decltype( out << value.value())
         {
            if constexpr( detail::has::valid< T>)
               if( ! value.valid())
                  return out << "nil";
            
            return out << value.value();
         } 
         
      } // detail::ostream


      template< typename T, typename Policy>
      inline auto operator << ( std::ostream& out, const Type< T, Policy>& rhs) 
         -> decltype( detail::ostream::stream( out, rhs, traits::priority::tag< 1>{}))
      { 
         return detail::ostream::stream( out, rhs, traits::priority::tag< 1>{});
      }

      template< typename T, typename Policy>
      inline auto operator >> ( std::istream& in, Type< T, Policy>& rhs) -> decltype( in >> rhs.underlying())
      { 
         return in >> rhs.underlying();
      }
      
   } // common::strong
} // casual

namespace std 
{
   template< casual::common::strong::detail::has::value_hash T>
   struct hash< T>
   {
      auto operator()( const T& value) const noexcept
      {
         return std::hash< std::remove_cvref_t< decltype( value.value())>>{}( value.value());
      }
   };
}
