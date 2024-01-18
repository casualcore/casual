//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "casual/platform.h"
#include "common/strong/type.h"
#include "common/uuid.h"

namespace casual
{
   namespace common::strong
   {
      namespace detail::integral
      {
         template< typename T, typename Tag, T initial>
         struct policy
         {
            constexpr static T initialize() noexcept { return initial;}
            constexpr static bool valid( const T& value) noexcept { return value != initial;}
         };

         //! just a helper for common integral id's
         template< typename T, typename Tag, T initial>
         using Type = strong::Type< T, policy< T, Tag, initial>>;

      } // detail::integral
         

      namespace process
      {
         struct tag{};
         using id = detail::integral::Type< platform::process::native::type, tag, -1>;

      } // process

      namespace ipc
      {
         struct tag{};
         using id = common::strong::Type< Uuid, tag>;

      } // ipc

      namespace file
      {
         namespace descriptor
         {
            struct tag{};
            using id = detail::integral::Type< platform::file::descriptor::native::type, tag, platform::file::descriptor::native::invalid>;
         
         } // descriptor
      } // file

      namespace socket
      {

         namespace detail
         {
            struct tag{};
            using base_type = strong::detail::integral::Type< platform::socket::native::type, tag, platform::socket::native::invalid>;
         } // detail
         
         struct id : detail::base_type
         {
            using detail::base_type::base_type;
            inline explicit id( file::descriptor::id value) : detail::base_type{ value.underlying()} {}

            //! implicit conversion to file descriptor
            inline operator file::descriptor::id () const { return file::descriptor::id{ value()};}
         };
         
      } // socket

      namespace resource
      {
         struct policy
         {
            constexpr static auto initialize() noexcept { return platform::resource::native::invalid;}
            constexpr static bool valid( platform::resource::native::type value) noexcept { return value != initialize();}
            static std::ostream& stream( std::ostream& out, platform::resource::native::type value);
            static platform::resource::native::type generate();
         };

         using id = strong::Type< platform::resource::native::type, policy>;

      } // resource

      namespace queue
      {
         struct tag{};
         using id = detail::integral::Type< platform::queue::native::type, tag, platform::queue::native::invalid>;
         
      } // queue

      namespace correlation
      {
         struct policy
         {
            using extended_equality = void;
            inline static auto generate() { return uuid::make();}
         };
         using id = strong::Type< Uuid, policy>;

         static_assert( strong::detail::has::value_hash< id>);

      } // correlation
      

      namespace execution
      {
         struct policy
         {
            using extended_equality = void;
            inline static auto generate() { return uuid::make();}
         };
         using id = strong::Type< Uuid, policy>;
      } // execution

      namespace domain
      {
         struct tag{};
         using id = strong::Type< Uuid, tag>;
      } // domain

      namespace conversation
      {
         namespace descriptor
         {
            struct tag{};
            using id = detail::integral::Type< platform::descriptor::type , tag, platform::descriptor::invalid>;
         } // descriptor

      } // conversation

   } // common::strong
} // casual
