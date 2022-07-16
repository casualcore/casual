//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/algorithm.h"

namespace casual
{
   namespace common
   {
      namespace move
      {
         template< typename Implementation>
         struct Pimpl
         {
            template< typename ...Args>
            explicit Pimpl( Args&&... args) : m_holder( std::make_unique< Implementation>( std::forward< Args>( args)...)) {}

            Implementation* operator -> () const noexcept { return m_holder.get();}
            Implementation& operator * () const noexcept{ return *m_holder;}

            explicit operator bool () const noexcept { return predicate::boolean( m_holder);}

         protected:
            Pimpl( std::unique_ptr< Implementation> holder) : m_holder{ std::move( holder)} {} 

            std::unique_ptr< Implementation> m_holder;
         };

         static_assert( std::is_nothrow_move_constructible_v< Pimpl< int>> && std::is_nothrow_move_assignable_v< Pimpl< int>>);
         static_assert( ! std::is_copy_constructible_v< Pimpl< int>> && ! std::is_copy_assignable_v< Pimpl< int>>);

         
      } // move

      template< typename Implementation>
      struct Pimpl : move::Pimpl< Implementation>
      {
         using base_type = move::Pimpl< Implementation>;

         template< typename ...Args>
         Pimpl( Args&&... args) : base_type{ std::forward< Args>( args)...}{}

         template< typename... Ts>
         static Pimpl create( Ts&&... ts) { }

         Pimpl( Pimpl&&) noexcept = default;
         Pimpl& operator = ( Pimpl&&) noexcept = default;

         //! Make a deep copy
         Pimpl( const Pimpl& other) : base_type{ std::make_unique< Implementation>( *other)} {}

         //! We need to overload non const copy-ctor, otherwise variadic ctor will take it.
         Pimpl( Pimpl& other) : base_type{ std::make_unique< Implementation>( *other)} {}

         //! Make a deep copy
         Pimpl& operator = ( const Pimpl& other)
         {
            Pimpl temporary{ other};
            *this = std::move( temporary);
            return *this;
         }
      };

      static_assert( std::is_nothrow_move_constructible_v< Pimpl< int>> && std::is_nothrow_move_assignable_v< Pimpl< int>>);
      static_assert( std::is_copy_constructible_v< Pimpl< int>> && std::is_copy_assignable_v< Pimpl< int>>);

   } // common
} // casual


