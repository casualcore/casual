//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_OPTIONAL_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_OPTIONAL_H_

#include "../../../../thirdparty/stl/optional.hpp"

#include "common/marshal/marshal.h"
#include <iosfwd>

namespace casual
{
   namespace common
   {
      using std::experimental::optional;

      namespace value
      {

         namespace policy
         {
            template< typename T, T empty_value, typename Tag = void>
            struct value_empty
            {
               constexpr static T initialize() { return empty_value;}
               constexpr static bool empty( const T& value) { return value == empty_value;}
               
            };

            struct stream
            {
               template< typename T>
               static std::ostream& print( std::ostream& out, T&& value) { return out << value;}
            };

         } // policy

         template< typename T, typename P, typename S = policy::stream>
         class basic_optional
         {
         public:
            using policy_type = P;
            using value_type = T;
            using stream_type = S;

            using iterator = value_type*;
            using const_iterator = const value_type*;

            constexpr basic_optional() = default;
            constexpr basic_optional( T value) : m_value{ std::move( value)} {}

            
            constexpr bool empty() const { return policy_type::empty( m_value);}
            constexpr explicit operator bool () const { return ! empty();}

            constexpr void clear() noexcept { m_value = policy_type::initialize();}

            
            constexpr const T& front() const { return m_value;}
            constexpr T& front() { return m_value;}

            constexpr const T& value() const { return m_value;}            

            constexpr const value_type* operator -> () const { return &m_value;}
            constexpr value_type* operator -> () { return &m_value;}
            constexpr const value_type& operator * () const&  { return m_value;}
            constexpr value_type& operator * () & { return m_value;}

            constexpr iterator begin() { return &m_value;}
            constexpr iterator end() { return empty() ? begin() : begin() + 1;}
            constexpr const_iterator begin() const { return &m_value;}
            constexpr const_iterator end() const { return empty() ? begin() : begin() + 1;}

            constexpr friend bool operator == ( const basic_optional& lhs, const basic_optional& rhs) { return lhs.m_value == rhs.m_value; }
            constexpr friend bool operator != ( const basic_optional& lhs, const basic_optional& rhs) { return ! ( lhs == rhs); }
            constexpr friend bool operator < ( const basic_optional& lhs, const basic_optional& rhs) { return lhs.m_value < rhs.m_value; }

            constexpr inline friend bool operator == ( const basic_optional& lhs, const value_type& rhs) { return lhs.m_value == rhs; }
            constexpr inline friend bool operator == ( const value_type& lhs, const basic_optional& rhs) { return lhs == rhs.m_values; }

            constexpr friend bool operator < ( const basic_optional& lhs, const value_type& rhs) { return lhs.m_value < rhs; }
            constexpr friend bool operator > ( const basic_optional& lhs, const value_type& rhs) { return lhs.m_value > rhs; }



            constexpr void swap( basic_optional& other ) // todo: type dependent noexcept
            {
               using std::swap;
               swap( m_value, other.m_value);
            }

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & m_value;
            })

            inline friend std::ostream& operator << ( std::ostream& out, const basic_optional& value) { return stream_type::print( out, value.m_value);}

         private:
            value_type m_value = policy_type::initialize();
         };



         template< typename T, T empty_value, typename Tag = void, typename S = policy::stream>
         using optional = basic_optional< T, policy::value_empty< T, empty_value, Tag>, S>;

      } // value

   } // common
} // casual

namespace std 
{
   template< typename T, typename P, typename S>
   struct hash< casual::common::value::basic_optional< T, P, S>>
   {
      using option_type = casual::common::value::basic_optional< T, P, S>;

     auto operator()( const option_type& value) const
     {
       return std::hash< typename option_type::value_type>{}( *value);
     }
   };
}

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_OPTIONAL_H_
