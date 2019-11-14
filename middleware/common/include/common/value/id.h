//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/compare.h"
#include "common/traits.h"

namespace casual
{
   namespace common
   {
      namespace value
      {
         namespace id
         {
            namespace detail
            {
               template< typename T, typename Tag, T start = 1>
               struct sequence
               {
                  using native_type = T;
                  constexpr static native_type next() 
                  { 
                     return value++;
                  }

                  constexpr static native_type initalized() { return start;} 
                  
                  static native_type value;
               };

               template< typename T, typename Tag, T start>
               T sequence< T, Tag, start>::value = start;
            } // detail


            template< typename ID, typename ID::value_type start = 1>
            struct sequence
            {
               constexpr static ID next() 
               { 
                  using native_type = typename ID::value_type;
                  return ID{ id::detail::sequence< native_type, ID, start>::next()};
               }

            };

            namespace policy
            {
               struct default_tag{};
               
               template< typename T, typename Tag = default_tag>
               struct default_initialize
               {
                  constexpr static T initialize() noexcept { return T();}
               };

               template< typename T, typename Tag = default_tag, T start = 1>
               struct unique_initialize
               {
                  using sequence = id::detail::sequence< T, Tag, start>;
                  constexpr static auto initialize() noexcept
                  { 
                     return sequence::next();
                  }

                  constexpr static auto moved() noexcept { return sequence::initalized();}
               };

            } // policy
         } // id

         //! "id" abstraction to help make "id-handling" more explicit, typesafe and
         //! enable overloading on a specific id-type
         template< typename T, typename P = id::policy::default_initialize< T>>
         class basic_id : public Compare< basic_id< T, P>>
         {
         public:
            using policy_type = P;
            using value_type = T;

            using traits = traits::type< value_type>;

            constexpr static bool nothrow_construct = traits::nothrow_construct
               && std::is_same< decltype( policy_type::initialize), T() noexcept>::value;

            constexpr basic_id() noexcept( nothrow_construct) : m_value{ policy_type::initialize()} {}
            constexpr explicit basic_id( common::traits::by_value_or_const_ref_t< T> value) noexcept( traits::nothrow_move_construct) : m_value{ std::move( value)} {}

            constexpr basic_id( basic_id&& rhs) noexcept( traits::nothrow_move_construct) : m_value{ std::exchange( rhs.m_value, selective_moved< policy_type>())} {};
            constexpr basic_id& operator = ( basic_id&& rhs) noexcept( traits::nothrow_move_assign) { m_value = std::exchange( rhs.m_value, selective_moved< policy_type>()); return *this;};
            constexpr basic_id( const basic_id&) noexcept( traits::nothrow_copy_construct) = default;
            constexpr basic_id& operator = ( const basic_id& rhs) noexcept( traits::nothrow_copy_assign) = default;

            //! return id by value if suitable. const ref otherwise.
            constexpr auto value() const noexcept -> common::traits::by_value_or_const_ref_t< T>
            { return m_value;}   
            
            //! for Compare< T>
            constexpr decltype( auto) tie() const noexcept { return value();}

            constexpr void swap( basic_id& other ) noexcept( traits::nothrow_move_assign)
            {
               using std::swap;
               swap( m_value, other.m_value);
            }

            // forward serialization
            CASUAL_FORWARD_SERIALIZE( m_value)

            value_type& underlaying() noexcept { return m_value;}

         protected:
               template< typename policy>
               using has_moved = decltype( policy::moved());

               template< typename policy>
               static auto selective_moved() -> 
                  std::enable_if_t< common::traits::detect::is_detected< has_moved, policy>::value, T>
               {
                  return policy::moved();
               }

               template< typename policy>
               static auto selective_moved() -> 
                  std::enable_if_t< ! common::traits::detect::is_detected< has_moved, policy>::value, T>
               {
                  return policy::initialize();
               }

            value_type m_value;
         };

      } // value
   } // common
} // casual


