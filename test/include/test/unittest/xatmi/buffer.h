//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/xatmi.h"

#include <string_view>
#include <memory>

namespace casual
{
   namespace test::unittest::xatmi::buffer
   {

      namespace detail
      {
         template< typename Type>
         struct Holder
         {
            template< typename S> // just to be able to rely on integer conversion
            Holder( S size)
               : data{ ::tpalloc( Type::name.data(), nullptr, size)}, size{ ::tptypes( data, nullptr, nullptr)}
            {}

            Holder() : Holder( 128L) {}

            Holder( Holder&& other)
               : data{ std::exchange( other.data, nullptr)}, size{ std::exchange( other.size, {})}
            {}
            Holder& operator = ( Holder&& other)
            {
               std::swap( data, other.data);
               std::swap( size, other.size);
            }

            ~Holder()
            {
               if( data)
                  ::tpfree( data);
            }

            auto begin() noexcept { return data;}
            auto end() noexcept { return data + size;}

            explicit operator bool() const noexcept { return data != nullptr;}

            char* data{};
            long size{};
         };

         struct x_octet
         {
            constexpr static std::string_view name{ "X_OCTET"};
         };
      } // detail

      
      using x_octet = detail::Holder< detail::x_octet>;

   } // test::unittest::xatmi::buffer
} // casual