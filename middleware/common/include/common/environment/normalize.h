//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/environment/string.h"

#include "common/serialize/archive/type.h"
#include "common/serialize/value.h"
#include "common/traits.h"


namespace casual
{
   namespace common::environment
   {
      namespace detail
      {
         template< typename Policy>
         struct Traverser 
         {
            inline constexpr static auto archive_type() { return serialize::archive::Type::static_order_type;}

            template< typename... Ts> 
            Traverser( Ts&&... ts) : m_policy{ std::forward< Ts>( ts)...} {}


            template< typename T>
            Traverser& operator >> ( T&& value)
            {
               serialize::value::read( *this, value, nullptr);
               return *this;
            }

            inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char*) 
            { 
               return { size, true};
            }
            inline void container_end( const char*) {} // no-op

            inline bool composite_start( const char*) { return true;}
            inline void composite_end(  const char*) {} // no-op

            template< typename T> 
            auto read( T&& value, const char*)
               -> std::enable_if_t< serialize::traits::is::archive::write::type_v< traits::remove_cvref_t< T>>, bool>
            { 
               return read( std::forward< T>( value));
            }

            bool read( std::string& value)
            {
               value = m_policy( value);
               return true;
            }

            template< typename T> 
            bool read( T&& value) { return false; } // no op
         
         private:
            Policy m_policy;
         };

         namespace policy
         {
            struct direct
            {
               std::string operator () ( std::string& value) const 
               {
                  return environment::string( std::move( value));
               }
            };

            struct cache
            {
               cache( const std::vector< environment::Variable>& local)
                  : m_local( local) {}

               std::string operator () ( std::string& value) const 
               {
                  return environment::string( std::move( value), m_local);
               }

               const std::vector< environment::Variable>& m_local;
            };

         } // policy
         
      } // detail

      //! normalizes `value` with regards to environment variables
      //!
      //! in essence, traverse the type and apply environment::string to all
      //! found strings in the datastructure
      //!
      //! @returns the `value` (depending on invocation, either rvalue or lvalue)
      template< typename T>
      decltype( auto) normalize( T&& value)
      {
         detail::Traverser< detail::policy::direct>{} >> value;
         return std::forward< T>( value);
      }

      //! normalizes `value` with regards to environment variables
      //! if the variable is found in `local` it's used, otherwise ask the 
      //! real environment.
      //!
      //! in essence, traverse the type and apply environment::string to all
      //! found strings in the datastructure
      //!
      //! @returns the `value` (depending on invocation, either rvalue or lvalue)
      template< typename T>
      decltype( auto) normalize( T&& value, const std::vector< environment::Variable>& local)
      {
         detail::Traverser< detail::policy::cache>{ local} >> value;
         return std::forward< T>( value);;
      }
      
      //! overload for `environment::Variable`, that will copy the normalized strings (instead of moving them).
      inline void normalize( std::vector< environment::Variable>& value, const std::vector< environment::Variable>& local)
      {
         algorithm::for_each( value, [&local]( auto& value)
         {
            // we copy
            value = environment::string( value, local);
         });
      }
      
   } // common::environment
} // casual