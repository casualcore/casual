//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_VALUE_ID_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_VALUE_ID_H_

#include "common/marshal/marshal.h"
#include "common/platform.h"

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
                  return { id::detail::sequence< native_type, ID, start>::next()};
               }
            };

            namespace policy
            {
               struct default_tag{};
               
               template< typename T, typename Tag = default_tag>
               struct default_initialize
               {
                  constexpr static T initialize() { return T();}
               };

               template< typename T, typename Tag = default_tag, T start = 1>
               struct unique_initialize
               {
                  using sequence = id::detail::sequence< T, Tag, start>;
                  constexpr static auto initialize() 
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
         class basic_id
         {
         public:
            using policy_type = P;
            using value_type = T;


            constexpr basic_id() = default;
            constexpr basic_id( T value) : m_value{ std::move( value)} {}

            //! return id by value if integral. const ref otherwise.
            constexpr auto value() const -> std::conditional_t< std::is_integral< T>::value, T, const T&>
            { return m_value;}   
            

            constexpr friend bool operator == ( const basic_id& lhs, const basic_id& rhs) { return lhs.m_value == rhs.m_value; }
            constexpr friend bool operator != ( const basic_id& lhs, const basic_id& rhs) { return ! ( lhs == rhs); }
            constexpr friend bool operator < ( const basic_id& lhs, const basic_id& rhs) { return lhs.m_value < rhs.m_value; }
            constexpr friend bool operator <= ( const basic_id& lhs, const basic_id& rhs) { return lhs.m_value <= rhs.m_value; }
            constexpr friend bool operator > ( const basic_id& lhs, const basic_id& rhs) { return lhs.m_value > rhs.m_value; }
            constexpr friend bool operator >= ( const basic_id& lhs, const basic_id& rhs) { return lhs.m_value >= rhs.m_value; }

            constexpr inline friend bool operator == ( const basic_id& lhs, const value_type& rhs) { return lhs.m_value == rhs; }
            constexpr inline friend bool operator == ( const value_type& lhs, const basic_id& rhs) { return lhs == rhs.m_values; }


            constexpr void swap( basic_id& other ) // todo: type dependent noexcept
            {
               using std::swap;
               swap( m_value, other.m_value);
            }

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & m_value;
            })

            inline friend std::ostream& operator << ( std::ostream& out, const basic_id& value) { return out << value.m_value;}

         protected:
            value_type m_value = policy_type::initialize();
         };

      } // value
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_VALUE_ID_H_
