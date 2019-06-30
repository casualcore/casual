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
         template< typename T>
         class basic_pimpl
         {
         public:

            using implementation_type = T;
            using holder_type = std::unique_ptr< implementation_type>;

            template< typename ...Args>
            basic_pimpl( Args&&... args) : m_holder( std::make_unique< implementation_type>( std::forward< Args>( args)...)) {}

            basic_pimpl( basic_pimpl&&) noexcept = default;
            basic_pimpl& operator = ( basic_pimpl&&) noexcept = default;

            ~basic_pimpl() = default;

            implementation_type* operator -> () const { return m_holder.get();}
            implementation_type& operator * () const { return *m_holder.get();}

            explicit operator bool () { return m_holder.get();}

         protected:
            basic_pimpl( holder_type&& holder) : m_holder{ std::move( holder)} {} 

            holder_type m_holder;
         };
      } // move

      template< typename T>
      struct basic_pimpl : public move::basic_pimpl< T>
      {
         using base_type = move::basic_pimpl< T>;
         using implementation_type = T;

         template< typename ...Args>
         basic_pimpl( Args&&... args) : base_type{ std::forward< Args>( args)...}{}

         basic_pimpl( basic_pimpl&&) noexcept = default;
         basic_pimpl& operator = ( basic_pimpl&&) noexcept = default;

         //! Make a deep copy
         basic_pimpl( const basic_pimpl& other) : base_type{ std::make_unique< implementation_type>( *other)} {}

         //! We need to overload non const copy-ctor, otherwise variadic ctor will take it.
         basic_pimpl( basic_pimpl& other) : base_type{ std::make_unique< implementation_type>( *other)} {}

         //! Make a deep copy
         basic_pimpl& operator = ( const basic_pimpl& other)
         {
            basic_pimpl temporary{ other};
            *this = std::move( temporary);
            return *this;
         }
      };

   } // common
} // casual


