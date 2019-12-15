//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/range.h"
#include "common/traits.h"
#include "common/optional.h"

namespace casual
{
   namespace common
   {
      namespace range
      {
         namespace adapter
         {
            template< typename F, typename R>
            struct iterator 
            {
               using tuple_type = std::tuple< R, R>;
               using next_type = F;

               using iterator_category = std::forward_iterator_tag;
               using difference_type = platform::size::type;
               using value_type = R;
               using pointer = R*;
               using reference = R&;

               iterator() = default;
               iterator( next_type next, R range) 
                  : m_next{ std::move( next)}, m_ranges{ m_next.value()( std::move( range))} {}

               constexpr reference operator * () noexcept { return std::get< 0>( m_ranges);}
               constexpr iterator& operator ++ () noexcept
               {
                  m_ranges = m_next.value()( std::get< 1>( m_ranges));
                  return *this;
               }

               friend bool operator == ( const iterator& lhs, const iterator& rhs)
               {
                  if( ! rhs.m_next)
                     return lhs.current().empty();
                  return rhs.current().empty();
               }

               friend bool operator != ( const iterator& lhs, const iterator& rhs) { return ! (lhs == rhs);}

            private:

               decltype( auto) current() const {  return std::get< 0>( m_ranges);}

               common::optional< next_type> m_next;
               tuple_type m_ranges{};
            };

            //!
            //! constructs a _range adapter_ that privide a _range interface_ for
            //! lazy "splitted" ranges. 
            //!
            //! @param next `functor` that takes a `range` and "split" it. has to return a tuple [next, rest]
            //!              
            //! @param range 
            //! @return a range ada
            //!
            template< typename F, typename R> 
            auto make( F&& next, R&& range) 
            {
               using iterator_type = adapter::iterator< traits::remove_cvref_t< F>, traits::remove_cvref_t< R>>;
               using range_type = common::Range< iterator_type>;
               return range_type{ iterator_type{ std::forward< F>( next), std::forward< R>( range)}, iterator_type{}};
            }
            
         } // adapter
      } // range
   } // common
} // casual