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
      namespace detail::traits
      {
         //! some helper traits to enable/disable behavior
         //! @{
         namespace check
         {
            template< typename T> 
            using bool_operator = decltype( &T::operator bool);

            template< typename P>
            using extended_equality = typename P::extended_equality;

            template< typename P>
            using initialize = decltype( P::initialize());

            template< typename P>
            using generate = decltype( P::generate());

            template< typename P, typename T>
            using static_valid = decltype( P::valid( std::declval< const T&>()));

            template< typename T>
            using valid = decltype( std::declval< const T&>().valid());

            template< typename T, typename V>
            using comparable_types = decltype( std::declval< const T&>() == std::declval< const V&>());
            
         } // check

         namespace has
         {
            template< typename T>
            inline constexpr bool bool_operator_v = common::traits::detect::is_detected_v< check::bool_operator, T>;

            template< typename P>
            inline constexpr bool extended_equality_v = common::traits::detect::is_detected_v< check::extended_equality, P>;

            template< typename P>
            inline constexpr bool initialize_v = common::traits::detect::is_detected_v< check::initialize, P>;

            template< typename P>
            inline constexpr bool generate_v = common::traits::detect::is_detected_v< check::generate, P>;

            template< typename P, typename T>
            inline constexpr bool static_valid_v = common::traits::detect::is_detected_v< check::static_valid, P, T>;

            template< typename T>
            inline constexpr bool valid_v = common::traits::detect::is_detected_v< check::valid, T>;

         } // has

         namespace enable
         {
            template< typename T, typename Policy, typename V>
            inline constexpr bool extended_equality_v = ! std::is_same_v< T, V> 
               && common::traits::detect::is_detected_v< check::comparable_types, T, V>
               && has::extended_equality_v< Policy>;

         } // enable

         //! @}
              
      } // detail::traits


      //! "id" abstraction to help make "id-handling" more explicit, typesafe and
      //! enable overloading on a specific id-type
      //! `Policy` can be 'empty' and just act as a `tag` to make the type unique
      template< typename T, typename Policy>
      class Type : public Compare< Type< T, Policy>>
      {
      public:

         using value_type = T;
         using policy_type = Policy;
         using value_traits = common::traits::type< value_type>;

         constexpr Type() noexcept = default;
         constexpr explicit Type( common::traits::by_value_or_const_ref_t< T> value) noexcept( value_traits::nothrow_move_construct) 
            : m_value{ std::move( value)} {}


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

         //! @returns true if the underlaying value is valid
         template< typename S = T, typename P = Policy, 
            std::enable_if_t< 
               detail::traits::has::bool_operator_v< S> || detail::traits::has::static_valid_v< P, S>>* = nullptr> 
         constexpr explicit operator bool() const noexcept
         {
            if constexpr( detail::traits::has::static_valid_v< P, S>)
               return P::valid( m_value);

            return static_cast< bool>( m_value);
         }

         //! @returns true if the underlaying value is valid
         template< typename S = T, typename P = Policy, 
            std::enable_if_t< 
               detail::traits::has::bool_operator_v< S> || detail::traits::has::static_valid_v< P, S>>* = nullptr> 
         constexpr bool valid() const noexcept
         {
            if constexpr( detail::traits::has::static_valid_v< P, S>)
               return P::valid( m_value);

            return static_cast< bool>( m_value);
         }

         //! enabled if the policy_type has a `generate()` static function defined.
         //! @returns `Type` that has a value generated by policy_type::generate()
         template< typename P = Policy, std::enable_if_t< detail::traits::has::generate_v< P>>* = nullptr> 
         static Type generate() noexcept
         {
            return Type{ P::generate()};
         }
         

         // forward serialization
         CASUAL_FORWARD_SERIALIZE( m_value)

         //! accessor for the underlying type by ref. Should only
         //! be used when interacting with 'C-api:s'
         //! @{
         value_type& underlaying() noexcept { return m_value;}
         const value_type& underlaying() const noexcept { return m_value;}
         //! @}

         //! for Compare< T>
         constexpr decltype( auto) tie() const noexcept { return value();}

      protected:


         constexpr static value_type initialize() noexcept
         {
            if constexpr( detail::traits::has::initialize_v< policy_type>)
               return policy_type::initialize();
            return value_type{};
         }

         value_type m_value = initialize();
      };


      //! If the underlaying type has equality operations defined for other types we expose
      //! these if the policy type has defined extended_equality
      //! @{
      template< typename T, typename Policy, typename V>
      inline auto operator == ( const Type< T, Policy>& lhs, const V& rhs) 
         -> std::enable_if_t< detail::traits::enable::extended_equality_v< T, Policy, V>, bool>
      { 
         return lhs.value() == rhs;
      }

      template< typename T, typename Policy, typename V>
      inline auto operator != ( const Type< T, Policy>& lhs, const V& rhs) 
         -> std::enable_if_t< detail::traits::enable::extended_equality_v< T, Policy, V>, bool>
      { 
         return ! ( lhs.value() == rhs);
      }

      template< typename V, typename T, typename Policy>
      inline auto operator == ( const V& lhs, const Type< T, Policy>& rhs) 
         -> std::enable_if_t< detail::traits::enable::extended_equality_v< T, Policy, V>, bool>
      { 
         return rhs.value() == lhs;
      }

      template< typename V, typename T, typename Policy>
      inline auto operator != ( const V& lhs, const Type< T, Policy>& rhs) 
         -> std::enable_if_t< detail::traits::enable::extended_equality_v< T, Policy, V>, bool>
      { 
         return ! ( rhs.value() == lhs);
      }

      //! @}

      namespace detail::ostream
      {
         template< typename T>
         auto stream( std::ostream& out, const T& value, common::traits::priority::tag< 1>) 
            -> decltype( void( typename T::policy_type::stream( out, value)), out)
         {
            typename T::policy_type::stream( out, value);
            return out;
         }

         template< typename T>
         auto stream( std::ostream& out, const T& value, common::traits::priority::tag< 0>) 
            -> decltype( out << value.value())
         {
            if constexpr( detail::traits::has::valid_v< T>)
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
      inline auto operator >> ( std::istream& in, Type< T, Policy>& rhs) -> decltype( in >> rhs.underlaying())
      { 
         return in >> rhs.underlaying();
      }

      namespace detail::hash
      {
         // some stuff to only enable std::hash specialization if the value type of
         // the option can be used with std::hash
         // if needed somewere else, maybe move this to traits?
         // @{
         template< typename T>
         using detail_has_hash = decltype( std::hash< T>{}( std::declval< T&>()));

         template< typename T>
         constexpr bool has_hash_v = common::traits::detect::is_detected_v< detail_has_hash, T>;

         template< typename Type, typename>
         using enable_helper = Type;

         template< typename Type, typename Key>
         using enable = enable_helper< Type, std::enable_if_t< has_hash_v< Key>>>;
         // @}
      } // detail::hash
      
   } // common::strong
} // casual

namespace std 
{
   template< typename T, typename P>
   struct hash< casual::common::strong::detail::hash::enable< casual::common::strong::Type< T, P>, std::remove_const_t< T>>>
   {
     auto operator()( const casual::common::strong::Type< T, P>& value) const 
     {
         return std::hash< std::remove_const_t< T>>{}( value.value());
     }
   };
}
