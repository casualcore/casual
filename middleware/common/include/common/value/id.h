//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_VALUE_ID_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_VALUE_ID_H_

#include "common/marshal/marshal.h"
#include "common/platform.h"
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
               };

            } // policy
         } // id



         //!
         //! "id" abstraction to help make "id-handling" more explicit, typesafe and
         //! enable overloading on a specific id-type
         //!
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
            constexpr explicit basic_id( T value) noexcept( traits::nothrow_move_construct) : m_value{ std::move( value)} {}

            constexpr basic_id( basic_id&& rhs) noexcept( traits::nothrow_move_construct) : m_value{ std::exchange( rhs.m_value, policy_type::initialize())} {};
            constexpr basic_id& operator = ( basic_id&& rhs) noexcept( traits::nothrow_move_assign) { m_value = std::exchange( rhs.m_value, policy_type::initialize()); return *this;};
            constexpr basic_id( const basic_id&) noexcept( traits::nothrow_copy_construct) = default;
            constexpr basic_id& operator = ( const basic_id& rhs) noexcept( traits::nothrow_copy_assign) = default;

            //! return id by value if integral. const ref otherwise.
            constexpr auto value() const -> std::conditional_t< std::is_integral< T>::value, T, const T&>
            { return m_value;}   
            

            //constexpr inline friend decltype( auto) tie( const basic_id& value) { return value.value();}
            constexpr inline decltype( auto) tie() const { return value();}


            constexpr void swap( basic_id& other ) noexcept( traits::nothrow_move_assign)
            {
               using std::swap;
               swap( m_value, other.m_value);
            }

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & m_value;
            })

            value_type& underlaying() { return m_value;}

            inline friend std::ostream& operator << ( std::ostream& out, const basic_id& value) { return out << value.m_value;}

         protected:
            value_type m_value;
         };

      } // value
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_VALUE_ID_H_