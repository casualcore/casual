//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include <utility>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace move
      {
         namespace policy
         {
            template< typename T, T active_value, T moved_value = T{}>
            struct Default 
            {
               constexpr static auto active() noexcept { return active_value;}
               constexpr static auto moved() noexcept { return moved_value;}
               constexpr static bool moved( const T& value) noexcept { return value == moved();} 
            };

            template< typename T>
            struct Pointer
            {
               constexpr static auto active() noexcept { return nullptr;}
               constexpr static auto moved() noexcept { return nullptr;}
               constexpr static bool moved( const T* value) noexcept { return value == moved();} 
            };


         } // policy

         template< typename T, T active_value = T{}, typename P = policy::Default< T, active_value>> 
         struct basic_active
         {
            using policy_type = P;

            basic_active() = default;
            basic_active( T value) : value{ std::move( value)} {}

            basic_active( basic_active&& other) noexcept
               : value{ std::exchange( other.value, policy_type::moved())} {}

            basic_active& operator = ( basic_active&& other) noexcept
            {
               value = std::exchange( other.value, policy_type::moved());
               return *this;
            }

            basic_active( basic_active&) = delete;
            basic_active( const basic_active&) = delete;
            basic_active& operator = ( const basic_active&) = delete;

            //! @return true if `value` is not moved
            explicit operator bool () const noexcept { return ! policy_type::moved( value);}

            auto release() noexcept { return std::exchange( value, policy_type::moved());}

            T value = policy_type::active();

            inline friend std::ostream& operator << ( std::ostream& out, const basic_active& value) { return out << value.value;}
            
         };

         //! indicator type to deduce if it has been moved or not.
         //!
         //! usecase:
         //!  Active as an attribute in <some type>
         //!  <some type> can use default move ctor and move assignment.
         //!  use Active attribute in dtor do deduce if instance of <some type> still
         //!  has responsibility...
         using Active = basic_active< bool, true>;

         template< typename T> 
         using Pointer = basic_active< T*, nullptr, policy::Pointer< T>>;

         
      } // move
   } // common
} // casual


