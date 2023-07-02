//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/range.h"
#include "common/traits.h"
#include "common/functional.h"

namespace casual
{
   namespace common::range::adapter
   {

      template< typename R>
      struct iterator 
      {
         using range_type = R;
         using tuple_type = std::tuple< R, R>;
         using next_callback = common::function< tuple_type( range_type)>;

         using iterator_category = std::forward_iterator_tag;
         using difference_type = platform::size::type;
         using value_type = R;
         using pointer = R*;
         using reference = R&;

         iterator() = default;
         iterator( next_callback next, R range) 
            : m_next{ std::move( next)}, m_ranges{ iterator::next( m_next, std::move( range))} {}
         
         const value_type& operator * () const noexcept { return std::get< 0>( m_ranges);}

         constexpr iterator& operator ++ () noexcept
         {
            m_ranges = iterator::next( m_next, std::get< 1>( m_ranges));
            return *this;
         }

         constexpr iterator operator ++ ( int) noexcept
         {
            auto result = *this;
            ++(*this);
            return result;
         }

         friend bool operator == ( const iterator& lhs, const iterator& rhs)
         {
            if( ! rhs.m_next)
               return lhs.empty();
            return rhs.empty();
         }

         friend bool operator != ( const iterator& lhs, const iterator& rhs) { return ! (lhs == rhs);}

      private:

         static tuple_type next( next_callback& next, value_type range)
         {
            return next( std::move( range));
         }

         bool empty() const noexcept
         {
            return std::get< 0>( m_ranges).empty() && std::get< 1>( m_ranges).empty();
         }

         next_callback m_next;
         tuple_type m_ranges{};

      };

      static_assert( std::movable< iterator< Range< const char*>>>);

      

      //!
      //! constructs a _range adapter_ that provide a _range interface_ for
      //! lazy "splitted" ranges. 
      //!
      //! @param next `functor` that takes a `range` and "split" it. has to return a tuple [next, rest]
      //!              
      //! @param range the range to "split"
      //! @return the _range adapter_
      //!
      template< typename F, typename R> 
      auto make( F&& next, R&& range) 
      {
         using iterator_type = adapter::iterator< std::remove_reference_t< R>>;
         using range_type = common::Range< iterator_type>;
         return range_type{ iterator_type{ std::forward< F>( next), std::forward< R>( range)}, iterator_type{}};
      }
      
   } // common::range::adapter
} // casual