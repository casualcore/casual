//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/value/id.h"
#include "common/stream.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace value
      {
         namespace optional
         {
            namespace policy
            {
               struct default_tag{};

               template< typename T, T empty_value, typename Tag = default_tag>
               struct value_empty
               {
                  constexpr static T initialize() noexcept { return empty_value;}
                  constexpr static bool empty( const T& value) noexcept { return value == empty_value;}
                  
               };

               struct stream
               {
                  template< typename T>
                  static std::ostream& print( std::ostream& out, bool valid, T&& value) 
                  { 
                     return valid ? out << value : out << "nil";
                  }
               };

            } // policy
         } // optional

         template< typename T, typename P, typename S = optional::policy::stream>
         class basic_optional : public common::value::basic_id< T, P>
         {
         public:
            using base_type = common::value::basic_id< T, P>;
            using policy_type = P;
            using value_type = T;
            using stream_type = S;

            using iterator = value_type*;
            using const_iterator = const value_type*;

            using base_type::base_type;
            
            constexpr bool empty() const { return policy_type::empty( this->m_value);}
            
            explicit constexpr operator bool () const { return ! empty();}

            constexpr bool has_value() const { return empty();}

            constexpr void clear() noexcept { this->m_value = policy_type::initialize();}
            
            inline friend std::ostream& operator << ( std::ostream& out, const basic_optional& optional) { return stream_type::print( out, ! optional.empty(), optional.value());}
         };



         template< typename T, T empty_value, typename Tag = optional::policy::default_tag, typename S = optional::policy::stream>
         using Optional = basic_optional< T, optional::policy::value_empty< T, empty_value, Tag>, S>;

         namespace detail
         {
            // some stuff to only enable std::hash specialization if the value type of
            // the option can be used with std::hash
            // if needed somewere else, maybe move this to traits?
            // @{
            template< typename T>
            using detail_has_hash = decltype( std::hash< T>{}( std::declval< T&>()));

            template< typename T>
            using has_hash = traits::detect::is_detected< detail_has_hash, T>;

            template< typename Type, typename>
            using enable_hash_helper = Type;

            template< typename Type, typename Key>
            using enable_hash = enable_hash_helper< Type, std::enable_if_t< has_hash< Key>::value>>;
            // @}

         } // detail
      } // value

   } // common
} // casual


namespace std 
{
   template< typename T, typename P, typename S>
   struct hash< casual::common::value::detail::enable_hash< casual::common::value::basic_optional< T, P, S>, std::remove_const_t< T>>>
   {
      using option_type = casual::common::value::basic_optional< T, P, S>;

     auto operator()( const option_type& value) const
     {
         return std::hash< std::remove_const_t< T>>{}( value.value());
     }
   };
}


