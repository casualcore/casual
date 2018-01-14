//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_VALUE_OPTIONAL_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_VALUE_OPTIONAL_H_

#include "common/value/id.h"

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
                     return valid ? out << value : out << "invalid";
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

            constexpr void clear() noexcept { this->m_value = policy_type::initialize();}

            
            constexpr decltype( auto) front() const { return this->value();}
            constexpr T& front() { return this->m_value;}
         

            constexpr const value_type* operator -> () const { return &this->m_value;}
            constexpr value_type* operator -> () { return &this->m_value;}
            constexpr const value_type& operator * () const&  { return this->m_value;}
            constexpr value_type& operator * () & { return this->m_value;}

            constexpr iterator begin() { return &this->m_value;}
            constexpr iterator end() { return empty() ? begin() : begin() + 1;}
            constexpr const_iterator begin() const { return &this->m_value;}
            constexpr const_iterator end() const { return empty() ? begin() : begin() + 1;}

            inline friend std::ostream& operator << ( std::ostream& out, const basic_optional& optional) { return stream_type::print( out, ! optional.empty(), optional.value());}
         };



         template< typename T, T empty_value, typename Tag = optional::policy::default_tag, typename S = optional::policy::stream>
         using Optional = basic_optional< T, optional::policy::value_empty< T, empty_value, Tag>, S>;

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



#endif
